
#include "hyb/ShaderProgramInstance.h"
#include "Terra.h"
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
    program.pushTexture(index++, pipeline.readImage(value), pipeline.getSampler(df.sampler));
    break;
  case ParamDeclTypeEnum::eWriteonlySSBO:
  case ParamDeclTypeEnum::eReadonlySSBO:
  case ParamDeclTypeEnum::eSSBO:
  {
    auto [buffer, size] = pipeline.readBuffer(value);
    program.pushBuffer(index++, buffer, 0, size);
  }
  break;
  case ParamDeclTypeEnum::eReadonlyImage:
    program.pushImage(index++, pipeline.readImage(value), 0, GfxAccess::eReadOnly, false);
    break;
  case ParamDeclTypeEnum::eImage:
    program.pushImage(index++, pipeline.readImage(value), 0, GfxAccess::eReadWrite, false);
    break;
  case ParamDeclTypeEnum::eWriteonlyImage:
    program.pushImage(index++, pipeline.readImage(value), 0, GfxAccess::eWriteOnly, false);
    break;
  case ParamDeclTypeEnum::eTextureBuffer:
  {
    auto [buffer, size] = pipeline.readBuffer(value);
    program.pushTexBuffer(index++, buffer, df.imageFormat);
  }
  break;
  }
}

void ShaderProgramInstance::pushOutput(HybridBuffer::handle value, bool clear, vec4 clearVal)
{
  program.pushOutput(index++, pipeline.readImage(value), clear, clearVal);
}

void ShaderProgramInstance::run()
{
  get().getDevice().postProcessDraw(program.program.material.program, program.program.material.layout, program.data);
}

} // namespace terra