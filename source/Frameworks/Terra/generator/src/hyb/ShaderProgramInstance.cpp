
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
    break;
  case DataTypeEnum::eInt2:
    program.pushScalar(index++, value.ivalue2);
    break;
  case DataTypeEnum::eUint:
    program.pushScalar(index++, value.uvalue);
    break;
  case DataTypeEnum::eUint2:
    program.pushScalar(index++, value.uvalue2);
    break;
  case DataTypeEnum::eFloat:
    program.pushScalar(index++, value.value);
    break;
  case DataTypeEnum::eFloat2:
    program.pushScalar(index++, value.value2);
    break;
  case DataTypeEnum::eFloat3:
    program.pushScalar(index++, value.value3);
    break;
  case DataTypeEnum::eFloat4:
    program.pushScalar(index++, value.value4);
    break;
  case DataTypeEnum::eMat4:
    program.pushScalar(index++, value.value4x4);
    break;
  }
}

void ShaderProgramInstance::pushImage(GfxImage::handle image, DataFormat df)
{
  program.pushTexture(index++, image, pipeline.getSampler(df.sampler));
}

void ShaderProgramInstance::pushBuffer(GfxBuffer::handle buffer, uint32_t size, DataFormat df)
{
  program.pushBuffer(index++, buffer, 0, size);
}

void ShaderProgramInstance::pushValue(HybridBuffer::handle value, DataFormat df)
{

  switch (df.declType)
  {
  case ParamDeclTypeEnum::eSampler1D:
  case ParamDeclTypeEnum::eSampler2D:
  case ParamDeclTypeEnum::eSampler1DArray:
  case ParamDeclTypeEnum::eSampler2DShadow:
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
  case ParamDeclTypeEnum::eReadonlyImage2D:
    program.pushImage(index++, pipeline.readImage(value), 0, GfxAccess::eReadOnly, false);
    break;
  case ParamDeclTypeEnum::eImage2D:
    program.pushImage(index++, pipeline.readImage(value), 0, GfxAccess::eReadWrite, false);
    break;
  case ParamDeclTypeEnum::eWriteonlyImage2D:
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

void ShaderProgramInstance::pushOutput(HybridBuffer::handle value, DataFormat format, bool clear, vec4 clearVal)
{
  auto const& sett = get().getSettings();

  if (format.declType == ParamDeclTypeEnum::eDepthOutput)
  {
    depth.clear    = clear;
    depth.depthVal = sett.reverseZ ? 1 - clearVal.x : clearVal.x;
    depth.image    = pipeline.writeImage(value, clear);
  }
  else
  {
    outputs[outputIdx].clear    = clear;
    outputs[outputIdx].colorVal = clearVal;
    outputs[outputIdx].image    = pipeline.writeImage(value, clear);

    outputIdx++;
  }
}

void ShaderProgramInstance::run()
{
  auto& dev  = get().getDevice();
  auto  pass = dev.createPass(std::span<GfxPass::Attachment>(outputs.data(), outputs.data() + outputIdx), depth);
  dev.setState(state);
  dev.beginPass(pass);
  dev.postProcessDraw(program.program.material.program, program.program.material.layout, program.data);
  dev.endPass();
  dev.destroy(pass);
}

} // namespace terra