#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.h"
#include "ComputeDevice.h"
#include "CurveData.h"
#include "DataSource.h"
#include "Dependency.h"
#include "GpuBuffer.h"
#include "Image.h"
#include "Serializer.h"

namespace terra
{
class Terra;

using Parameter = std::variant<ScalarValue, Source>;
using TaskKey   = uint64_t;
enum class Result
{
  eFinished,
  eContinue,
  eAbort
};

using ParamSetter = void (*)(Node&, Parameter&&);
using ParamGetter = Parameter const& (*)(Node const&);

template <DataType Type, DataType Scalar = DataType::eFloat>
struct FmtVal
{
  static inline constexpr DataType type   = Type;
  static inline constexpr DataType scalar = Scalar;

  static inline constexpr bool is_enum = false;

  DataValue defaultVal = {};
  DataValue minVal     = {-1.0f};
  DataValue maxVal     = {1.0f};
  DataValue stepVal    = {0.1f};

  FmtVal() = default;
  FmtVal(float iDef, float iMin = {-1.0f}, float iMax = {1.0f}, float iStep = {0.1f})
      : defaultVal(iDef), minVal(iMin), maxVal(iMax), stepVal(iStep)
  {}
  FmtVal(int iDef, int iMin = std::numeric_limits<int>::min(), int iMax = std::numeric_limits<int>::max(),
         int iStep = 1)
      : defaultVal(iDef), minVal(iMin), maxVal(iMax), stepVal(iStep)
  {}

  static inline constexpr auto get()
  {
    return DataFormat(type, scalar);
  }
};

struct FmtEnum
{
  static inline constexpr DataType type   = DataType::eEnum;
  static inline constexpr DataType scalar = DataType::eInt;

  static inline constexpr bool is_enum = true;

  DataValue                       defaultVal = {};
  std::vector<std::u8string_view> enumVals   = {};

  FmtEnum() = default;
  FmtEnum(int iDef, std::initializer_list<std::u8string_view> enums) : defaultVal(iDef), enumVals(enums) {}

  static inline constexpr auto get()
  {
    return DataFormat(type, scalar);
  }
};

struct ParameterMeta
{
  enum ValueType
  {
    eDefault = 0,
    eMin,
    eMax,
    eStep,
    eCount
  };

  DataFormat format;
  DrawHint   drawHint = DrawHint::eDefault;

  Semantic           semantic = Semantic::eNone;
  std::u8string_view name;
  std::u8string_view help;
  std::u8string_view tooltip;

  DataValue                       values[ValueType::eCount] = {};
  std::vector<std::u8string_view> enumValues                = {};

  ParamSetter setter = nullptr;
  ParamGetter getter = nullptr;

  ParameterMeta()                                         = default;
  ParameterMeta(ParameterMeta const&)                     = default;
  ParameterMeta(ParameterMeta&&) noexcept                 = default;
  ParameterMeta& operator=(ParameterMeta const&) noexcept = default;
  ParameterMeta& operator=(ParameterMeta&&) noexcept      = default;

  template <typename MembPtr, typename Fmt>
  ParameterMeta(MembPtr, Fmt format, std::u8string_view iname, std::u8string_view ihelp, std::u8string_view itooltip,
                Semantic semantic = Semantic::eNone, DrawHint idrawhint = DrawHint::eDefault)
      : format(Fmt::get()), name(iname), help(ihelp), tooltip(itooltip), drawHint(idrawhint),
        setter(
          [](Node& node, Parameter&& param)
          {
            using T               = typename MembPtr::class_t;
            Parameter T::*pmember = MembPtr::pmem;
            auto&         cnode   = static_cast<T&>(node);
            cnode.*pmember        = std::move(param);
          }),
        getter(
          [](Node const& node) -> Parameter const&
          {
            using T               = typename MembPtr::class_t;
            Parameter T::*pmember = MembPtr::pmem;
            auto&         cnode   = static_cast<T const&>(node);
            return cnode.*pmember;
          })
  {
    values[ValueType::eDefault] = format.defaultVal;
    if constexpr (Fmt::is_enum)
    {
      enumValues = std::move(format.enumVals);
    }
    else
    {
      values[ValueType::eMin]  = format.minVal;
      values[ValueType::eMax]  = format.maxVal;
      values[ValueType::eStep] = format.stepVal;
    }
  }

  inline bool isValid() const
  {
    return format.type != DataType::eInvalid;
  }

  bool canBeSource() const;
  bool canBeScalar() const;

  ScalarValue getDefault() const;

  void setTypeFromString(std::string_view);
  void setValueFromString(ValueType, std::string_view);
};

class Node;
class NodeMeta
{

public:
  struct ShaderContent
  {
    std::string function;
    std::string extensions;
    std::string shaderContent;
  };

  std::u8string_view         icon;
  std::u8string_view         name;
  std::u8string_view         category;
  std::u8string_view         tooltip;
  std::u8string_view         help;
  std::string_view           style;
  std::vector<ParameterMeta> parameterDef;
  uint32_t                   outputUpscale   = 1; // multiplier
  uint32_t                   outputDownscale = 1; // divisor for reduction algo
  DataFormat                 format          = DataFormat(DataType::eBuffer);

  // derived
  uint32_t id;
  bool     cacheResults = false;

  using Create = std::shared_ptr<Node> (*)(NodeMeta const&);

  // override run
  Create create = nullptr;

  // attributes
  bool attribTileConstrained = false;
  bool attribIteration       = false;
  //

  NodeMeta()                                    = default;
  NodeMeta(NodeMeta const&)                     = default;
  NodeMeta(NodeMeta&&) noexcept                 = default;
  NodeMeta& operator=(NodeMeta const&) noexcept = default;
  NodeMeta& operator=(NodeMeta&&) noexcept      = default;
};

class Node : public DataSource
{
public:
  std::u8string   name;
  NodeMeta const& meta;

  Node(NodeMeta const& m) : meta(m), name(m.name) {}

  uint32 getNumParams() const
  {
    return (uint32)meta.parameterDef.size();
  }

  Parameter const& param(uint32_t i) const
  {
    return meta.parameterDef[i].getter(*this);
  }

  Parameter param(uint32_t i, Parameter&& sv) 
  {
    auto old = meta.parameterDef[i].getter(*this);
    meta.parameterDef[i].setter(*this, std::move(sv));
    return old;
  }

  Parameter resetValue(uint32_t i)
  {
    auto old = meta.parameterDef[i].getter(*this);
    meta.parameterDef[i].setter(*this, meta.parameterDef[i].getDefault());
    return old;
  }

  virtual Type getType() const
  {
    return Type::eNode;
  }

  virtual DataFormat getFormat() const
  {
    return DataFormat(DataType::eBuffer);
  }

  void accept(dshandle source, Event) {}
};

} // namespace terra