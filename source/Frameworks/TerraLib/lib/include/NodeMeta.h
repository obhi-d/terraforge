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

// using ParamSetter = void (*)(Node&, Parameter&&);
// using ParamGetter = Parameter const& (*)(Node const&);

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

  Semantic    semantic = Semantic::eNone;
  DisplayInfo displayInfo;

  DataValue                       values[ValueType::eCount] = {};
  std::vector<std::u8string_view> enumValues                = {};

  // ParamSetter setter = nullptr;
  // ParamGetter getter = nullptr;

  ParameterMeta()                                         = default;
  ParameterMeta(ParameterMeta const&)                     = default;
  ParameterMeta(ParameterMeta&&) noexcept                 = default;
  ParameterMeta& operator=(ParameterMeta const&) noexcept = default;
  ParameterMeta& operator=(ParameterMeta&&) noexcept      = default;

  template <typename Fmt>
  ParameterMeta(Fmt format, std::string_view iname, Semantic semantic = Semantic::eNone,
                DrawHint idrawhint = DrawHint::eDefault)
      : format(Fmt::get()), drawHint(idrawhint)
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
  std::vector<ParameterMeta> parameterDef    = {ParameterMeta(FmtVal<DataType::eInput>(), "@input")};
  uint32_t                   outputUpscale   = 1; // multiplier
  uint32_t                   outputDownscale = 1; // divisor for reduction algo
  DataFormat                 format          = DataFormat(DataType::eBuffer);

  // derived
  uint32_t id;
  bool     cacheResults = false;

  // attributes
  bool attribTileConstrained = false;
  bool attribIteration       = false;
  //

  NodeMeta()                                    = default;
  NodeMeta(NodeMeta const&)                     = default;
  NodeMeta(NodeMeta&&) noexcept                 = default;
  NodeMeta& operator=(NodeMeta const&) noexcept = default;
  NodeMeta& operator=(NodeMeta&&) noexcept      = default;

  virtual void prepareGeneration(Node&, Pipeline&) const = 0;
  virtual void beginIteration(Node&, Pipeline&) const           = 0;
  virtual void endIteration(Node&, Pipeline&) const      = 0;
};

} // namespace terra