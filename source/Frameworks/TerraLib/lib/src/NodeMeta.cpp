
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

bool DataFormat::isCompatible(DataFormat const& from, DataFormat const& to)
{
  switch (from.type)
  {
  case DataType::eInt2:
  case DataType::eFloat2:
  case DataType::eInt:
  case DataType::eFloat:
  case DataType::eCurveData:
  case DataType::eBool:
  case DataType::eEnum:
    return from.type == to.type;
  // case DataType::eImageSource:
  case DataType::eImage:
    return (to.type == DataType::eImage) && from.scalarSubType == to.scalarSubType;
  case DataType::eInput:
  case DataType::eBuffer:
    return from.type == to.type && from.scalarSubType == to.scalarSubType;
  }
  return false;
}

std::string_view typeToString(DataType type)
{
  switch (type)
  {
  case DataType::eEnum:
    return "enum";
  case DataType::eInt:
    return "int";
  case DataType::eInt2:
    return "ivec2";
  case DataType::eFloat:
    return "float";
  case DataType::eFloat2:
    return "vec2";
  case DataType::eBool:
    return "bool";
  case DataType::eImage:
    return "image";
  case DataType::eBuffer:
    return "source";
  case DataType::eInput:
    return "input";
  case DataType::eCurveData:
    return "curve";
  }
  return "invalid";
}

DataType stringToType(std::string_view stype)
{
  DataType type = DataType::eInvalid;
  if (stype == "enum")
    type = DataType::eEnum;
  else if (stype == "int")
    type = DataType::eInt;
  else if (stype == "float")
    type = DataType::eFloat;
  else if (stype == "ivec2")
    type = DataType::eInt2;
  else if (stype == "vec2")
    type = DataType::eFloat2;
  else if (stype == "bool")
    type = DataType::eBool;
  else if (stype == "image")
    type = DataType::eImage;
  else if (stype == "buffer" || stype == "source")
    type = DataType::eBuffer;
  else if (stype == "input")
    type = DataType::eInput;
  else if (stype == "curve")
    type = DataType::eCurveData;
  return type;
}

bool ParameterMeta::canBeSource() const
{
  switch (format.type)
  {
  case DataType::eCurveData:
  case DataType::eBuffer:
  case DataType::eImage:
  case DataType::eInput:
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
  if (DataType::eBool == format.type)
    return ScalarValue((bool)(values[ValueType::eDefault].ival != 0));
  else if (DataType::eEnum == format.type)
    return ScalarValue(values[ValueType::eDefault].ival, 0);
  switch (format.scalarSubType)
  {
  case DataType::eFloat:
    return ScalarValue(values[ValueType::eDefault].fval);
  case DataType::eFloat2:
    return vec2{values[ValueType::eDefault].fval, values[ValueType::eDefault].fval};
  case DataType::eBool:
  case DataType::eInt:
    return ScalarValue(values[ValueType::eDefault].ival);
  case DataType::eInt2:
    return ivec2{values[ValueType::eDefault].ival, values[ValueType::eDefault].ival};
  default:
    return ScalarValue();
  }
}

void ParameterMeta::setTypeFromString(std::string_view stype)
{
  format.type = stringToType(stype);
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto setter = [this, valType](auto value)
  {
    values[valType] = DataValue(value);
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (format.type)
  {
  case DataType::eEnum:
  case DataType::eBool:
  case DataType::eInt2:
  case DataType::eInt:
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
    setter(ivalue);
    break;
  case DataType::eCurveData:
  case DataType::eImage:
  case DataType::eInput:
  case DataType::eBuffer:
  case DataType::eFloat:
  case DataType::eFloat2:
    if (value == "inf")
      fvalue = std::numeric_limits<float>::infinity();
    else if (value == "-inf")
      fvalue = -std::numeric_limits<float>::infinity();
    else
      std::from_chars(value.data(), value.data() + value.size(), fvalue);
    setter(fvalue);
    break;
  }
}

NodeMeta::NodeMeta()
{
  parameterDef.emplace_back(MemberPtr<&Node::domain>(), "@domain", FmtVal<DataType::eInput>());
}

void NodeMeta::addDomain()
{
  parameterDef.emplace_back(MemberPtr<&Node::domain>(), "@domain", FmtVal<DataType::eInput>());
}
} // namespace terra