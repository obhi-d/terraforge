#pragma once
#include <acl/dynamic_array.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.h"
#include "CurveData.h"
#include "DataSource.h"
#include "Dependency.h"
#include "GfxDevice.h"
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

inline bool store(HDataSource& s, Parameter param)
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
using ParamSetter = void (*)(Node&, uint32_t i, Parameter);
using ParamGetter = Parameter (*)(Node const&, uint32_t i);

struct ValueRange
{
  DataValue defaultVal = {};
  DataValue minVal     = {-1.0f};
  DataValue maxVal     = {1.0f};
  DataValue stepVal    = {0.1f};

  inline ValueRange() = default;
  inline ValueRange(DataValue def, DataValue minV, DataValue maxV, DataValue stepV)
      : defaultVal(def), minVal(minV), maxVal(maxV), stepVal(stepV)
  {}
};

struct FmtEnum
{
  static inline constexpr DataTypeEnum      type        = DataTypeEnum::eEnum;
  static inline constexpr DataTypeEnum      scalar      = DataTypeEnum::eInt;
  static inline constexpr ParamDeclTypeEnum declType    = ParamDeclTypeEnum::eNone;
  static inline constexpr ImageFormatEnum   imageFormat = ImageFormatEnum::eNone;

  static inline constexpr bool is_enum = true;

  DataValue                     defaultVal = {};
  std::vector<std::string_view> enumVals   = {};

  FmtEnum() = default;
  FmtEnum(int iDef, std::initializer_list<std::string_view> enums) : defaultVal(iDef), enumVals(enums) {}

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

  DisplayInfo displayInfo;

  DataFormat                     format;
  std::unique_ptr<DisplayInfo[]> enumDisplayInfo = {};
  ValueRange                     ranges;

  union
  {
    uint32_t maxEnum = 1;
    uint32_t maxOption;
  };
  ParamSetter setter = nullptr;
  ParamGetter getter = nullptr;

  ParameterMeta() = default;
  ParameterMeta(ParameterMeta const& other)
  {
    *this = other;
  }
  ParameterMeta(ParameterMeta&&) noexcept = default;
  ParameterMeta& operator=(ParameterMeta const& other) noexcept
  {
    displayInfo = other.displayInfo;
    format      = other.format;
    maxEnum     = other.maxEnum;
    setter      = other.setter;
    getter      = other.getter;
    ranges      = other.ranges;
    if (other.enumDisplayInfo)
    {
      enumDisplayInfo.reset(new DisplayInfo[maxEnum]);
      for (uint32_t i = 0; i < maxEnum; ++i)
        enumDisplayInfo[i] = other.enumDisplayInfo[i];
    }
    return *this;
  }
  ParameterMeta& operator=(ParameterMeta&&) noexcept = default;

  template <typename MembPtr>
  ParameterMeta(MembPtr, std::string_view iname, ValueRange values, DataTypeEnum type = DataTypeEnum::eBuffer,
                DataTypeEnum subType = DataTypeEnum::eFloat, ImageFormatEnum imageFmt = ImageFormatEnum::eFloat,
                ParamDeclTypeEnum declType = ParamDeclTypeEnum::eSampler2D, SemanticEnum semantic = SemanticEnum::eNone,
                SamplerParamEnum sampler = SamplerParamEnum::eNone, bool preEval = false)
      : format(type, subType, imageFmt, declType, semantic, sampler, preEval), ranges(values), displayInfo(iname),
        setter(
          [](Node& node, uint32_t, Parameter param)
          {
            using T       = typename MembPtr::class_t;
            auto  pmember = MembPtr::pmem;
            auto& cnode   = static_cast<T&>(node);
            store(cnode.*pmember, param);
          }),
        getter(
          [](Node const& node, uint32_t) -> Parameter
          {
            using T       = typename MembPtr::class_t;
            auto  pmember = MembPtr::pmem;
            auto& cnode   = static_cast<T const&>(node);
            return Parameter(cnode.*pmember);
          })
  {}

  template <typename MembPtr>
  ParameterMeta(MembPtr, std::string_view iname, std::initializer_list<std::string_view> enums, int defEn = 0)
      : format(DataTypeEnum::eEnum, DataTypeEnum::eInt, ImageFormatEnum::eNone, ParamDeclType::eNone,
               SemanticEnum::eNone, false),
        displayInfo(iname), setter(
                              [](Node& node, uint32_t, Parameter param)
                              {
                                using T       = typename MembPtr::class_t;
                                auto  pmember = MembPtr::pmem;
                                auto& cnode   = static_cast<T&>(node);
                                store(cnode.*pmember, param);
                              }),
        getter(
          [](Node const& node, uint32_t) -> Parameter
          {
            using T       = typename MembPtr::class_t;
            auto  pmember = MembPtr::pmem;
            auto& cnode   = static_cast<T const&>(node);
            return Parameter(cnode.*pmember);
          })
  {

    maxEnum = (uint32)enums.size();
    enumDisplayInfo.reset(new DisplayInfo[maxEnum]);
    for (uint32_t i = 0; i < maxEnum; ++i)
    {
      enumDisplayInfo[i].from(enums[i]);
    }
  }

  std::string_view name() const
  {
    return displayInfo.id;
  }

  inline bool isValid() const
  {
    return format.type != DataTypeEnum::eInvalid;
  }

  bool canBeSource() const;
  bool canBeScalar() const;

  ScalarValue getDefault() const;

  void setTypeFromString(std::string_view type, std::string_view scalarType);
  void setValueFromString(ValueType, std::string_view);
  void setDeclFromString(std::string_view type);
};

class Node;

struct AutoParam
{
  enum Result
  {
    eContinueIteration,
    eReturnResult,
    eOk,
    eReportFailure
  };
  using Callback = Result (*)(Pipeline&, Node&, uint32_t);
  Callback pre   = nullptr;
  Callback post  = nullptr;
  AutoParam()    = default;
  AutoParam(Callback ipre, Callback ipost) : pre(ipre), post(ipost) {}
};

struct OutputMeta
{
  DisplayInfo displayInfo;
  DataFormat  format     = DataFormat(DataTypeEnum::eBuffer);
  vec4        clearValue = vec4(0.f);
  bool        clear      = false;

  std::string_view name() const
  {
    return displayInfo.id;
  }

  inline OutputMeta() = default;

  inline OutputMeta(std::string_view name)
  {
    displayInfo.from(name);
  }

  inline OutputMeta(std::string_view name, DataFormat fmt) : format(fmt)
  {
    displayInfo.from(name);
  }
};

class NodeMeta
{

public:
  uint32_t    icon  = 0;
  uint32_t    style = 0;
  DisplayInfo displayInfo;

  std::vector<ParameterMeta> parameterDef;

  std::vector<uint32_t> categorySorted;
  std::vector<uint32_t> autoParams;
  std::vector<uint32_t> autoOutputs;

  std::vector<OutputMeta> outputs;
  CreateNode              createNode = nullptr;

  // derived
  uint32_t id;
  bool     cacheResults = false;

  template <typename N>
  void as()
  {
    createNode = [](NodeMeta const& param) -> std::shared_ptr<Node>
    {
      return std::static_pointer_cast<Node>(std::make_shared<N>(param));
    };
  }

  NodeMeta()                                    = default;
  NodeMeta(NodeMeta const&)                     = default;
  NodeMeta(NodeMeta&&) noexcept                 = default;
  NodeMeta& operator=(NodeMeta const&) noexcept = default;
  NodeMeta& operator=(NodeMeta&&) noexcept      = default;

  std::string_view name() const
  {
    return displayInfo.id;
  }

  virtual void prepare();

  static void registerAuto(SemanticEnum e, AutoParam param)
  {
    if ((uint32_t)e >= autoRegistry.size())
      autoRegistry.resize((uint32_t)e + 1);
    autoRegistry[(uint32_t)e] = param;
  }

  static void registerAuto(SemanticEnum e, AutoParam::Callback pre, AutoParam::Callback post)
  {
    registerAuto(e, AutoParam(pre, post));
  }

  using AutoParamRegistry = std::vector<AutoParam>;
  static AutoParamRegistry autoRegistry;
};

struct SemanticContext
{
  GfxImage::handle height;
  // water, rocks, vegetation, color;
  GfxImage::handle layerContrib;
};

} // namespace terra