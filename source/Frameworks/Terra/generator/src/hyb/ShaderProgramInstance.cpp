
#include "hyb/ShaderProgramInstance.h"
#include "hyb/HybridPipeline.h"

namespace terra
{
void ShaderProgramInstance::pushValue(ScalarValue value, DataTypeEnum type)
{
  switch (type)
  {
  case DataTypeEnum::eInt:
    program.pushScalar(index++, value.ivalue);
  case DataTypeEnum::eInt2:
    program.pushScalar(index++, value.ivalue2);
  case DataTypeEnum::eUint:
    program.pushScalar(index++, value.uvalue);
  case DataTypeEnum::eUint2:
    program.pushScalar(index++, value.uvalue2);
  case DataTypeEnum::eFloat:
    program.pushScalar(index++, value.value);
  case DataTypeEnum::eFloat2:
    program.pushScalar(index++, value.value2);
  }
}

void ShaderProgramInstance::pushValue(HybridBuffer::handle value, DataFormat df)
{
  switch (df.declType)
  {
  case ParamDeclTypeEnum::eTexture:
    program.pushTexture(pipeline.readImage(value),
  }
}

} // namespace terra