
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
  if (DataTypeEnum::eBool == format.type)
    return (bool)(ranges.defaultVal.ival != 0);
  else if (DataTypeEnum::eEnum == format.type)
    return ranges.defaultVal.ival;
  switch (format.scalarSubType)
  {
  case DataTypeEnum::eFloat:
    return ranges.defaultVal.fval;
  case DataTypeEnum::eFloat2:
    return vec2{ranges.defaultVal.fval};
  case DataTypeEnum::eFloat3:
    return vec3{ranges.defaultVal.fval};
  case DataTypeEnum::eFloat4:
    return vec4{ranges.defaultVal.fval};
  case DataTypeEnum::eMat4:
  {
    auto m4 = mat4{ranges.defaultVal.fval};
    return std::make_shared<ArrayFloat>(&m4[0][0], &m4[0][0] + 16);
  }
  case DataTypeEnum::eInt:
    return ranges.defaultVal.ival;
  case DataTypeEnum::eInt2:
    return ivec2{ranges.defaultVal.ival, ranges.defaultVal.ival};
  case DataTypeEnum::eUint:
    return (uint32_t)ranges.defaultVal.ival;
  case DataTypeEnum::eUint2:
    return uvec2((uint32_t)ranges.defaultVal.ival);
  default:
    return float(0.0f);
  }
}

void ParameterMeta::setTypeFromString(std::string_view type, std::string_view scalarType)
{
  format.type          = DataType::fromString(type);
  format.scalarSubType = DataType::fromString(scalarType);
}

void ParameterMeta::setDeclFromString(std::string_view type)
{
  format.declType = ParamDeclType::fromString(type);
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto localSetter = [this, valType](auto value)
  {
    switch (valType)
    {
    case ValueType::eDefault:
      ranges.defaultVal = DataValue(value);
      break;
    case ValueType::eMax:
      ranges.maxVal = DataValue(value);
      break;
    case ValueType::eMin:
      ranges.minVal = DataValue(value);
      break;
    case ValueType::eStep:
      ranges.stepVal = DataValue(value);
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