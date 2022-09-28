
#include "Node.h"
#include <charconv>

namespace terra
{
void ParameterMeta::setTypeFromString(std::string_view type)
{
  if (type == "int")
    type = ParameterType::eInt;
  else if (type == "float")
    type = ParameterType::eFloat;
  else if (type == "int2")
    type = ParameterType::eInt2;
  else if (type == "float2")
    type = ParameterType::eFloat2;
  else if (type == "bool")
    type = ParameterType::eBool;
  else if (type == "image")
    type = ParameterType::eImage;
  else if (type == "source")
    type = ParameterType::eDataSource;
  else if (type == "curve")
    type = ParameterType::eCurveData;
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto setter = [this, valType](auto value)
  {
    switch (valType)
    {
    case ValueType::eDefault:
      idefault = *(int*)(&value);
      break;
    case ValueType::eMin:
      imin = *(int*)(&value);
      break;
    case ValueType::eMax:
      imax = *(int*)(&value);
      break;
    }
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (type)
  {
  case ParameterType::eInt2:
  case ParameterType::eInt:
    std::from_chars(value.data(), value.data() + value.size(), ivalue);
    setter(ivalue);
    break;
  case ParameterType::eDataSource:
  case ParameterType::eFloat:
  case ParameterType::eFloat2:
    std::from_chars(value.data(), value.data() + value.size(), fvalue);
    setter(fvalue);
    break;
  }
}

} // namespace terra