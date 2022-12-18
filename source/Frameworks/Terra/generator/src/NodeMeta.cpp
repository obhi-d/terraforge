
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

ScalarValue ParameterMeta::getDefault() const
{
  if (DataTypeEnum::eBool == format.type)
    return ScalarValue((bool)(values[ValueType::eDefault].ival != 0));
  else if (DataTypeEnum::eEnum == format.type)
    return ScalarValue(values[ValueType::eDefault].ival, 0);
  switch (format.scalarSubType)
  {
  case DataTypeEnum::eFloat:
    return ScalarValue(values[ValueType::eDefault].fval);
  case DataTypeEnum::eFloat2:
    return vec2{values[ValueType::eDefault].fval, values[ValueType::eDefault].fval};
  case DataTypeEnum::eBool:
  case DataTypeEnum::eInt:
    return ScalarValue(values[ValueType::eDefault].ival);
  case DataTypeEnum::eInt2:
    return ivec2{values[ValueType::eDefault].ival, values[ValueType::eDefault].ival};
  default:
    return ScalarValue();
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
    values[valType] = DataValue(value);
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (format.scalarSubType)
  {
  case DataTypeEnum::eEnum:
  case DataTypeEnum::eBool:
  case DataTypeEnum::eInt2:
  case DataTypeEnum::eInt:
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

NodeMeta::NodeMeta()
{
  parameterDef.emplace_back(MemberPtr<&Node::domain>(), "@domain", FmtVal<DataTypeEnum::eInput>());
}

void NodeMeta::addDomain()
{
  parameterDef.emplace_back(MemberPtr<&Node::domain>(), "@domain", FmtVal<DataTypeEnum::eInput>());
}

void NodeMeta::prepare()
{
  for (uint32_t i = 0, end = (uint32_t)parameterDef.size(); i < end; ++i)
  {
    auto autoIdx = (uint32_t)parameterDef[i].format.semantic;
    if (autoIdx != (uint32_t)SemanticEnum::eNone)
    {
      if (autoIdx < autoRegistry.size())
      {
        if (autoRegistry[autoIdx].pre || autoRegistry[autoIdx].post)
          autoParams.emplace_back(i);
      }
    }
  }

  for (uint32_t i = 0, end = (uint32_t)outputs.size(); i < end; ++i)
  {
    auto autoIdx = (uint32_t)outputs[i].format.semantic;
    if (autoIdx != (uint32_t)SemanticEnum::eNone)
    {
      if (autoIdx < autoRegistry.size())
      {
        if (autoRegistry[autoIdx].pre || autoRegistry[autoIdx].post)
          autoOutputs.emplace_back(i);
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