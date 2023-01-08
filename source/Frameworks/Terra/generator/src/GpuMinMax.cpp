
#include "GpuMinMax.h"
#include "ResourceUtils.h"
#include "Terra.h"

namespace terra
{

ShaderProgramPtr GpuMinMax::texturePass;
ShaderProgramPtr GpuMinMax::bufferPass;
ShaderProgramPtr GpuMinMax::normalizePass;

void GpuMinMax::buildProgram()
{
  auto code = fileContentToString("shaders/reduce.glsl");
  {
    auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, SourceType::eComputeProgram);

    builder->option("Pass_Texture");
    builder->param("texture_size_x", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("block_size", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("pixel_count", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("data_src",
                   DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
    builder->param("data_dst", DataFormat(DataType::eBuffer, DataType::eFloat, ImageFormat::eFloat,
                                          ParamDeclType::eWriteonlyStorageBuffer));
    builder->append(code);
    texturePass = builder->finalize();
  }
  {
    auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, SourceType::eComputeProgram);

    builder->param("block_size", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("pixel_count", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("skip_block_size", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("data_buffer",
                   DataFormat(DataType::eBuffer, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eStorageBuffer));

    builder->append(code);
    bufferPass = builder->finalize();
  }

  {
    auto norm    = fileContentToString("shaders/normalize.glsl");
    auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, SourceType::eComputeProgram);

    builder->option("Pass_Normalize");
    builder->param("source",
                   DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eImage2D));
    builder->param("data_buffer",
                   DataFormat(DataType::eBuffer, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eStorageBuffer));

    builder->append(code);
    normalizePass = builder->finalize();
  }
}

void GpuMinMax::destroy()
{
  texturePass   = {};
  bufferPass    = {};
  normalizePass = {};
}

vec2 GpuMinMax::execute(GfxImage::handle image, glm::uvec2 size, uint32_t block)
{
  // create scratch buffer
  auto& dev = get().getDevice();

  uint32_t totalPixels       = size.x * size.y;
  uint32_t scratchBufferSize = (totalPixels + block - 1) / block;
  uint32_t bufferSize        = scratchBufferSize * 8;
  auto     buffer            = dev.createBuffer(GfxStorageClass::eDeviceAccess, GfxBuffer::fStorage, bufferSize);

  dev.barrier(GfxBarrierFlags::fTextureAccess);
  // texture pass
  {
    auto tex = ShaderMaterial(*texturePass);
    tex.pushScalar(size.x);
    tex.pushScalar(block);
    tex.pushScalar(size.x * size.y);
    tex.pushTexture(image, {});
    tex.pushBuffer(buffer, 0, bufferSize);
    // texture pass
    dev.dispatchCompute(tex.get(), scratchBufferSize, 1);
  }

  {

    // reduce pass
    auto     buff = ShaderMaterial(*bufferPass);
    uint32_t skip = 1;
    while (scratchBufferSize > 1)
    {
      buff.reset();
      uint32_t index = 0;
      buff.pushScalar(block);
      buff.pushScalar(scratchBufferSize);
      buff.pushScalar(skip);
      buff.pushBuffer(buffer, 0, bufferSize);
      scratchBufferSize = (scratchBufferSize + block - 1) / block;
      dev.barrier(GfxBarrierFlags::fStorageBuffer);
      dev.dispatchCompute(buff.get(), scratchBufferSize, 1);
      skip *= block;
    }
  }

  // final normalize pass
  {
    auto norm = ShaderMaterial(*normalizePass);
    norm.pushImage(image, 0, GfxAccess::eReadWrite, false);
    norm.pushBuffer(buffer, 0, 8);
    dev.barrier(GfxBarrierFlags::fStorageBuffer);
    dev.dispatchCompute(norm.get(), size.x, size.y);
  }
  dev.destroy(buffer);

  return vec2{-1.0f, 1.0f};
}

} // namespace terra