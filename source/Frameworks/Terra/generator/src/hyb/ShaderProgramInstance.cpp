
#include "hyb/ShaderProgramInstance.h"
#include "Terra.h"
#include "hyb/HybridPipeline.h"

namespace terra
{
void ShaderProgramInstance::pushValue(Parameter const& value, DataTypeEnum type, DataTypeEnum subtype)
{
  switch (type)
  {
  case DataTypeEnum::eBuffer:
    pushValue(value, subtype, subtype);
    break;
  case DataTypeEnum::eMat4:
  case DataTypeEnum::eArray:
    switch (subtype)
    {
    case DataTypeEnum::eFloat2:
    case DataTypeEnum::eFloat3:
    case DataTypeEnum::eFloat4:
    case DataTypeEnum::eMat4:
    case DataTypeEnum::eFloat:
      program.pushArray(toSpan(*std::get<ArrayFloatRef>(value)));
      break;
    case DataTypeEnum::eInt2:
    case DataTypeEnum::eInt:
      program.pushArray(toSpan(*std::get<ArrayIntRef>(value)));
      break;
    case DataTypeEnum::eUint2:
    case DataTypeEnum::eUint:
      program.pushArray(toSpan(*std::get<ArrayUintRef>(value)));
      break;
    }
    break;
  case DataTypeEnum::eInt:
    program.pushScalar(std::get<int>(value));
    break;
  case DataTypeEnum::eInt2:
    program.pushScalar(std::get<ivec2>(value));
    break;
  case DataTypeEnum::eUint:
    program.pushScalar(std::get<uint32_t>(value));
    break;
  case DataTypeEnum::eUint2:
    program.pushScalar(std::get<uvec2>(value));
    break;
  case DataTypeEnum::eFloat:
    program.pushScalar(std::get<float>(value));
    break;
  case DataTypeEnum::eFloat2:
    program.pushScalar(std::get<vec2>(value));
    break;
  case DataTypeEnum::eFloat3:
    program.pushScalar(std::get<vec3>(value));
    break;
  case DataTypeEnum::eFloat4:
    program.pushScalar(std::get<vec4>(value));
    break;
  }
}

void ShaderProgramInstance::pushImage(GfxImage::handle image, DataFormat df)
{
  barrier = (GfxBarrierFlags)(barrier | GfxBarrierFlags::fImageAccess | GfxBarrierFlags::fTextureAccess);
  program.pushTexture(image, pipeline.getSampler(df.sampler));
}

void ShaderProgramInstance::pushBuffer(GfxBuffer::handle buffer, uint32_t size, DataFormat df)
{
  barrier = (GfxBarrierFlags)(barrier | GfxBarrierFlags::fStorageBuffer);
  program.pushBuffer(buffer, 0, size);
}

void ShaderProgramInstance::pushValue(HybridBuffer::handle value, DataFormat df)
{

  switch (df.declType)
  {
  case ParamDeclTypeEnum::eSampler1D:
  case ParamDeclTypeEnum::eSampler2D:
  case ParamDeclTypeEnum::eSampler1DArray:
  case ParamDeclTypeEnum::eSampler2DShadow:
    program.pushTexture(pipeline.readImage(value), pipeline.getSampler(df.sampler));
    break;
  case ParamDeclTypeEnum::eWriteonlyStorageBuffer:
  case ParamDeclTypeEnum::eReadonlyStorageBuffer:
  case ParamDeclTypeEnum::eStorageBuffer:
  {
    auto [buffer, size] = pipeline.readBuffer(value);
    program.pushBuffer(buffer, 0, size);
  }
  break;
  case ParamDeclTypeEnum::eReadonlyImage2D:
    program.pushImage(pipeline.readImage(value), 0, GfxAccess::eReadOnly, false);
    break;
  case ParamDeclTypeEnum::eImage2D:
    program.pushImage(pipeline.readImage(value), 0, GfxAccess::eReadWrite, false);
    break;
  case ParamDeclTypeEnum::eWriteonlyImage2D:
    program.pushImage(pipeline.readImage(value), 0, GfxAccess::eWriteOnly, false);
    break;
  case ParamDeclTypeEnum::eTextureBuffer:
  {
    auto [buffer, size] = pipeline.readBuffer(value);
    program.pushTexBuffer(buffer, df.imageFormat);
  }
  break;
  }
}

void ShaderProgramInstance::pushOutput(HybridBuffer::handle value, DataFormat format, bool clear, vec4 clearVal)
{
  if (isComputePass)
  {
    program.pushImage(pipeline.writeImage(value, clear), 0,
                      format.declType == ParamDeclTypeEnum::eImage2D ? GfxAccess::eReadWrite : GfxAccess::eWriteOnly,
                      false);
  }
  else
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
}

void ShaderProgramInstance::run()
{
  auto& dev = get().getDevice();
  if (isComputePass)
  {
    dev.barrier(barrier);
    dev.dispatchCompute(program.get(), pipeline.size().x, pipeline.size().y);
    dev.barrier(barrier);
  }
  else
  {
    auto pass = dev.createPass(std::span<GfxPass::Attachment>(outputs.data(), outputs.data() + outputIdx), depth);
    dev.setState(state);
    dev.beginPass(pass);
    dev.postProcessDraw(program.get());
    dev.endPass();
    dev.destroy(pass);
  }
}

} // namespace terra