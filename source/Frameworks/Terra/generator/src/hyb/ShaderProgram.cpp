
#include "hyb/ShaderProgram.h"

namespace terra
{

void ShaderProgramInstance::pushValue(ScalarValue value, DataTypeEnum type)
{

  switch (type)
  {
  case DataTypeEnum::eFloat:
    pushValue(value.value);
    break;
  case DataTypeEnum::eFloat2:
    pushValue(value.value2);
    break;
  case DataTypeEnum::eInt:
    pushValue(value.ivalue);
    break;
  case DataTypeEnum::eInt2:
    pushValue(value.ivalue2);
    break;
  }
}

} // namespace terra