#pragma once
#include <acl/dynamic_array.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ParamHelper.h"
#include "Serializer.h"

namespace terra
{
class Terra;

template <typename T>
inline bool store(T& s, Parameter const& param)
{
  if (std::holds_alternative<T>(param))
  {
    s = std::get<T>(param);
    return true;
  }
  return false;
}

inline bool store(Parameter& s, Parameter const& param)
{
  s = param;
  return true;
}

inline bool store(Source& s, Parameter const& param)
{
  if (std::holds_alternative<Source>(param))
  {
    s = std::get<Source>(param);
    return true;
  }
  return false;
}

inline bool store(HDataSource& s, Parameter const& param)
{
  if (std::holds_alternative<Source>(param))
  {
    s = std::get<Source>(param).source;
    return true;
  }
  return false;
}

inline bool store(Angle& s, Parameter const& param)
{
  if (std::holds_alternative<float>(param))
  {
    s = std::get<float>(param);
    return true;
  }
  return false;
}

inline bool store(Unorm& s, Parameter const& param)
{
  if (std::holds_alternative<float>(param))
  {
    s = std::get<float>(param);
    return true;
  }
  return false;
}

inline bool store(Snorm& s, Parameter const& param)
{
  if (std::holds_alternative<float>(param))
  {
    s = std::get<float>(param);
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

using EnumData   = std::vector<DisplayInfo>;
using StringData = std::u8string_view;
using RangeData  = ValueRange;

struct ButtonData
{
  std::u8string_view toggleTxt;
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

  DataFormat format;

  std::variant<EnumData, StringData, RangeData, ButtonData> contents;

  uint64_t dependencies = 0;

  union
  {

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
    contents    = other.contents;
    setter      = other.setter;
    getter      = other.getter;
    return *this;
  }
  ParameterMeta& operator=(ParameterMeta&&) noexcept = default;

  template <typename MembPtr>
  ParameterMeta(MembPtr, std::string_view iname, ValueRange values, DataTypeEnum type = DataTypeEnum::eBuffer,
                DataTypeEnum subType = DataTypeEnum::eFloat, ImageFormatEnum imageFmt = ImageFormatEnum::eFloat,
                ParamDeclTypeEnum declType = ParamDeclTypeEnum::eSampler2D, Semantic semantic = {},
                SamplerParamEnum sampler = SamplerParamEnum::eNone, bool preEval = false)
      : format(type, subType, imageFmt, declType, semantic, sampler, preEval), contents(values), displayInfo(iname),
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
      : format(DataTypeEnum::eEnum, DataTypeEnum::eInt, ImageFormatEnum::eNone, ParamDeclType::eNone, Semantic{},
               false),
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
    contents = EnumData(enums.begin(), enums.end());
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

  Parameter getDefault() const;

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
    eSkipPass,
    ePauseExecution,
    eReportFailure
  };

  using CallbackPre    = Result (*)(Pipeline&, Node&, uint32_t, Parameter&);
  using CallbackPost   = Result (*)(Pipeline&, Node&, uint32_t);
  using CallbackChange = void (*)(Node&, uint32_t);
  using CallbackPass   = Result (*)(Pipeline&, Node&, uint32_t pass);

  CallbackPre    pre    = nullptr;
  CallbackPost   post   = nullptr;
  CallbackChange change = nullptr;
  CallbackPass   pass   = nullptr;

  std::vector<Semantic> naturalDeps;

  AutoParam() = default;
  AutoParam(CallbackPre ipre, CallbackPost ipost) : pre(ipre), post(ipost) {}
  AutoParam(CallbackChange ichange, std::initializer_list<Semantic> deps)
      : change(ichange), naturalDeps(std::move(deps))
  {}
  AutoParam(CallbackPass ipass) : pass(ipass) {}
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
  uint64_t              preParams   = 0;
  uint64_t              depParams   = 0;
  uint64_t              postParams  = 0;
  uint64_t              autoOutputs = 0;

  std::vector<OutputMeta> outputs;
  CreateNode              createNode = nullptr;

  // derived
  uint32_t id;
  bool     cacheResults = false;

  uint32_t paramIdx(Semantic sem)
  {
    auto it = std::find_if(parameterDef.begin(), parameterDef.end(),
                           [sem](auto& def)
                           {
                             return (def.format.semantic == sem);
                           });
    if (it != parameterDef.end())
      return static_cast<uint32_t>(std::distance(parameterDef.begin(), it));
    return 0xffffffff;
  }

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

  static void registerAuto(Semantic e, AutoParam param)
  {
    if ((uint32_t)e.id >= autoRegistry.size())
      autoRegistry.resize((uint32_t)e.id + 1);
    autoRegistry[(uint32_t)e.id] = param;
  }

  static void registerAuto(Semantic e, AutoParam::CallbackPre pre, AutoParam::CallbackPost post)
  {
    registerAuto(e, AutoParam(pre, post));
  }

  static void registerAuto(Semantic e, AutoParam::CallbackChange c, std::initializer_list<Semantic> deps)
  {
    registerAuto(e, AutoParam(c, std::move(deps)));
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