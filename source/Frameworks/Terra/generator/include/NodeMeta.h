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

inline bool store(Parameter& s, Parameter param)
{
  s = param;
  return true;
}

inline bool store(Source& s, Parameter param)
{
  if (std::holds_alternative<Source>(param))
  {
    s = std::get<Source>(param);
    return true;
  }
  return false;
}

inline bool store(dshandle& s, Parameter param)
{
  if (std::holds_alternative<Source>(param))
  {
    s = std::get<Source>(param).source;
    return true;
  }
  return false;
}

inline bool store(float& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).value;
    return true;
  }
  return false;
}

inline bool store(int& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).ivalue;
    return true;
  }
  return false;
}

inline bool store(bool& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).bvalue;
    return true;
  }
  return false;
}

inline bool store(vec2& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).value2;
    return true;
  }
  return false;
}

inline bool store(ivec2& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).ivalue2;
    return true;
  }
  return false;
}

inline bool store(Angle& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).value;
    return true;
  }
  return false;
}

inline bool store(Unorm& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).value;
    return true;
  }
  return false;
}

inline bool store(Snorm& s, Parameter param)
{
  if (std::holds_alternative<ScalarValue>(param))
  {
    s = std::get<ScalarValue>(param).value;
    return true;
  }
  return false;
}

using TaskKey = uint64_t;
enum class Result
{
  eFinished,
  eContinue,
  eAbort
};

class NodeMeta;
struct ParameterMeta;
using CreateNode  = std::shared_ptr<Node> (*)(NodeMeta const&);
using ParamSetter = void (*)(Node&, Parameter);
using ParamGetter = Parameter (*)(Node const&);

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

template <typename MembPtr>
inline auto DeriveFormat()
{

  if constexpr (std::is_same_v<typename MembPtr::member_t, dshandle>)
    return FmtVal<DataType::ePostProcess>();
  else if constexpr (std::is_same_v<typename MembPtr::member_t, Parameter> ||
                     std::is_same_v<typename MembPtr::member_t, Source>)
    return FmtVal<DataType::eBuffer>(0.0f, -std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity());
  else if constexpr (std::is_same_v<typename MembPtr::member_t, bool>)
    return FmtVal<DataType::eBool>();
  else if constexpr (std::is_same_v<typename MembPtr::member_t, float>)
    return FmtVal<DataType::eFloat>(0.0f, -std::numeric_limits<float>::infinity(),
                                    std::numeric_limits<float>::infinity());
  else if constexpr (std::is_same_v<typename MembPtr::member_t, int>)
    return FmtVal<DataType::eInt>(0, -std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  else if constexpr (std::is_same_v<typename MembPtr::member_t, vec2>)
    return FmtVal<DataType::eFloat2>(0.0f, -std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity());
  else if constexpr (std::is_same_v<typename MembPtr::member_t, ivec2>)
    return FmtVal<DataType::eInt2>(0, -std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  else if constexpr (std::is_same_v<typename MembPtr::member_t, Angle>)
    return FmtVal<DataType::eFloat>(0.0f, -180.f, 180.f);
  else if constexpr (std::is_same_v<typename MembPtr::member_t, Unorm>)
    return FmtVal<DataType::eFloat>(0.0f, 0.f, 1.f, 0.01f);
  else if constexpr (std::is_same_v<typename MembPtr::member_t, Snorm>)
    return FmtVal<DataType::eFloat>(0.0f, -1.f, 1.f, 0.05f);
}

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
  DisplayInfo displayInfo;

  DataValue                       values[ValueType::eCount] = {};
  std::vector<std::u8string_view> enumValues                = {};

  ParamSetter setter = nullptr;
  ParamGetter getter = nullptr;

  ParameterMeta()                                         = default;
  ParameterMeta(ParameterMeta const&)                     = default;
  ParameterMeta(ParameterMeta&&) noexcept                 = default;
  ParameterMeta& operator=(ParameterMeta const&) noexcept = default;
  ParameterMeta& operator=(ParameterMeta&&) noexcept      = default;

  template <typename MembPtr>
  ParameterMeta(MembPtr, std::string_view iname, Semantic semantic = Semantic::eNone,
                DrawHint idrawhint = DrawHint::eDefault)
      : ParameterMeta(MembPtr(), iname, DeriveFormat<MembPtr>(), semantic, idrawhint)
  {}

  template <typename MembPtr, typename Fmt>
  ParameterMeta(MembPtr, std::string_view iname, Fmt format, Semantic semantic = Semantic::eNone,
                DrawHint idrawhint = DrawHint::eDefault)
      : format(Fmt::get()), drawHint(idrawhint), setter(
                                                   [](Node& node, Parameter param)
                                                   {
                                                     using T       = typename MembPtr::class_t;
                                                     auto  pmember = MembPtr::pmem;
                                                     auto& cnode   = static_cast<T&>(node);
                                                     store(cnode.*pmember, param);
                                                   }),
        getter(
          [](Node const& node) -> Parameter
          {
            using T       = typename MembPtr::class_t;
            auto  pmember = MembPtr::pmem;
            auto& cnode   = static_cast<T const&>(node);
            return Parameter(cnode.*pmember);
          })
  {
    displayInfo.from(iname);
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

  std::string_view           icon;
  DisplayInfo                displayInfo;
  std::u8string_view         category;
  std::string_view           style;
  std::vector<ParameterMeta> parameterDef;

  std::vector<DataFormat> outputs    = {DataFormat(DataType::eBuffer)};
  CreateNode              createNode = nullptr;

  // derived
  uint32_t id;
  bool     cacheResults = false;

  // attributes
  bool attribTileConstrained = false;
  bool attribIteration       = false;
  //

  template <typename N>
  void as()
  {
    createNode = [](NodeMeta const& param) -> std::shared_ptr<Node>
    {
      return std::static_pointer_cast<Node>(std::make_shared<N>(param));
    };
  }

  void addDomain();

  NodeMeta(NoDomain) {}
  NodeMeta();
  NodeMeta(NodeMeta const&)                     = default;
  NodeMeta(NodeMeta&&) noexcept                 = default;
  NodeMeta& operator=(NodeMeta const&) noexcept = default;
  NodeMeta& operator=(NodeMeta&&) noexcept      = default;

  virtual void prepareGeneration(Node&, Pipeline&) const = 0;
  virtual void beginIteration(Node&, Pipeline&) const    = 0;
  virtual void endIteration(Node&, Pipeline&) const      = 0;
};

} // namespace terra