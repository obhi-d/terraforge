
#include "Logger.h"
#include "Node.h"
#include "Pipeline.h"
#include "ShaderBuilder.h"
#include "Terra.h"
#include "Updater.h"
#include <charconv>
#include <numeric>

namespace terra
{

NodeMeta::AutoParamRegistry NodeMeta::autoRegistry;

bool ParameterMeta::canBeSource() const
{
  switch (format.type)
  {
  case DataTypeEnum::eCurveData:
  case DataTypeEnum::eBuffer:
  case DataTypeEnum::eImage:
  case DataTypeEnum::eInput:
    return true;
  }
  return false;
}

bool ParameterMeta::canBeScalar() const
{
  return true;
}

Parameter ParameterMeta::getDefault() const
{
  static_assert(DataTypeEnum::kCount == 21);
  if (DataTypeEnum::eButton == format.type)
    return Parameter(Button{});
  else if (DataTypeEnum::eString == format.type)
    return Parameter(std::make_shared<std::string>());
  else if (DataTypeEnum::eBool == format.type)
    return (bool)(std::get<RangeData>(contents).defaultVal.ival != 0);
  else if (DataTypeEnum::eEnum == format.type)
    return (0);
  switch (format.scalarSubType)
  {
  case DataTypeEnum::eFloat:
    return (std::get<RangeData>(contents).defaultVal.fval);
  case DataTypeEnum::eFloat2:
    return vec2{(std::get<RangeData>(contents).defaultVal.fval)};
  case DataTypeEnum::eFloat3:
    return vec3{(std::get<RangeData>(contents).defaultVal.fval)};
  case DataTypeEnum::eFloat4:
    return vec4{(std::get<RangeData>(contents).defaultVal.fval)};
  case DataTypeEnum::eMat4:
  {
    auto m4 = mat4{(std::get<RangeData>(contents).defaultVal.fval)};
    return std::make_shared<ArrayFloat>(&m4[0][0], &m4[0][0] + 16);
  }
  case DataTypeEnum::eInt:
    return (std::get<RangeData>(contents).defaultVal.ival);
  case DataTypeEnum::eInt2:
    return ivec2{(std::get<RangeData>(contents).defaultVal.ival), (std::get<RangeData>(contents).defaultVal.ival)};
  case DataTypeEnum::eUint:
    return (uint32_t)(std::get<RangeData>(contents).defaultVal.ival);
  case DataTypeEnum::eUint2:
    return uvec2((uint32_t)(std::get<RangeData>(contents).defaultVal.ival));
  default:
    return float(0.0f);
  }
}

ParamDeclTypeEnum declTypeFromType(DataTypeEnum type)
{
  switch (type)
  {
  case DataTypeEnum::eBuffer:
    return ParamDeclTypeEnum::eStorageBuffer;
  case DataTypeEnum::eSource:
  case DataTypeEnum::eImage:
  case DataTypeEnum::ePostProcess:
  case DataTypeEnum::eInput:
    return ParamDeclTypeEnum::eSampler2D;
  default:
    return ParamDeclTypeEnum::eScalar;
  }
}

void ParameterMeta::setTypeFromString(std::string_view type, std::string_view scalarType)
{
  format.type          = DataType::fromString(type);
  format.scalarSubType = DataType::fromString(scalarType);
  format.declType      = declTypeFromType(format.type);
}

void ParameterMeta::setDeclFromString(std::string_view type)
{
  format.declType = ParamDeclType::fromString(type);
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto localSetter = [this, valType](auto value)
  {
    if (!std::holds_alternative<RangeData>(contents))
      contents = RangeData();
    switch (valType)
    {
    case ValueType::eDefault:
      std::get<RangeData>(contents).defaultVal = DataValue(value);
      break;
    case ValueType::eMax:
      std::get<RangeData>(contents).maxVal = DataValue(value);
      break;
    case ValueType::eMin:
      std::get<RangeData>(contents).minVal = DataValue(value);
      break;
    case ValueType::eStep:
      std::get<RangeData>(contents).stepVal = DataValue(value);
      break;
    }
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (format.scalarSubType)
  {
  case DataTypeEnum::eEnum:
  case DataTypeEnum::eBool:
  case DataTypeEnum::eInt2:
  case DataTypeEnum::eInt:
  case DataTypeEnum::eUint:
  case DataTypeEnum::eUint2:
    if (value == "inf")
      ivalue = std::numeric_limits<int>::max();
    else if (value == "-inf")
      ivalue = std::numeric_limits<int>::min();
    else if (value == "true")
      ivalue = 1;
    else if (value == "false")
      ivalue = 0;
    else
      std::from_chars(value.data(), value.data() + value.size(), ivalue);
    localSetter(ivalue);
    break;
  case DataTypeEnum::eCurveData:
  case DataTypeEnum::eImage:
  case DataTypeEnum::eInput:
  case DataTypeEnum::eBuffer:
  case DataTypeEnum::eFloat:
  case DataTypeEnum::eFloat2:
  case DataTypeEnum::eFloat3:
  case DataTypeEnum::eFloat4:
  case DataTypeEnum::eMat4:
    if (value == "inf")
      fvalue = std::numeric_limits<float>::infinity();
    else if (value == "-inf")
      fvalue = -std::numeric_limits<float>::infinity();
    else
      std::from_chars(value.data(), value.data() + value.size(), fvalue);
    localSetter(fvalue);
    break;
  }
}

void NodeMeta::prepare()
{
  for (uint32_t i = 0, end = (uint32_t)parameterDef.size(); i < end; ++i)
  {
    auto autoIdx = parameterDef[i].format.semantic;
    if (autoIdx)
    {
      if (autoIdx.id < autoRegistry.size())
      {
        if (autoRegistry[autoIdx.id].pre)
          preParams |= 1ull << i;
        if (autoRegistry[autoIdx.id].post)
          postParams |= 1ull << i;
        if (autoRegistry[autoIdx.id].change)
        {
          depParams |= 1ull << i;
          for (auto d : autoRegistry[autoIdx.id].naturalDeps)
          {
            uint32_t pd = paramIdx(d);
            if (pd != 0xffffffff)
              parameterDef[pd].dependencies |= 1ull << i;
          }
        }
      }
    }
  }

  for (uint32_t i = 0, end = (uint32_t)outputs.size(); i < end; ++i)
  {
    auto autoIdx = outputs[i].format.semantic;
    if (autoIdx)
    {
      if (autoIdx.id < autoRegistry.size())
      {
        if (autoRegistry[autoIdx.id].post)
          autoOutputs |= 1ull << i;
      }
    }
  }

  categorySorted.resize(parameterDef.size());
  std::iota(categorySorted.data(), categorySorted.data() + categorySorted.size(), 0);
  std::sort(categorySorted.begin(), categorySorted.end(),
            [this](uint32_t a, uint32_t b)
            {
              return parameterDef[a].displayInfo.category < parameterDef[b].displayInfo.category;
            });
}
} // namespace terra