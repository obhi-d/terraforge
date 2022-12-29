
#include "GpuMinMax.h"
#include "ResourceUtils.h"
#include "Terra.h"

namespace terra
{

ShaderProgramPtr GpuMinMax::texturePass;
ShaderProgramPtr GpuMinMax::bufferPass;

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
    builder->param("data_dst",
                   DataFormat(DataType::eBuffer, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eWriteonlySSBO));
    builder->append(code);
    texturePass = builder->finalize();
  }
  {
    auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, SourceType::eComputeProgram);

    builder->param("block_size", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("pixel_count", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("skip_block_size", DataFormat(DataType::eUint, DataType::eUint));
    builder->param("data_buffer",
                   DataFormat(DataType::eBuffer, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSSBO));

    builder->append(code);
    bufferPass = builder->finalize();
  }
}

void GpuMinMax::destroy()
{
  texturePass = {};
  bufferPass  = {};
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
    auto     tex   = ShaderMaterial(*texturePass);
    uint32_t index = 0;
    tex.pushScalar(index++, size.x);
    tex.pushScalar(index++, block);
    tex.pushScalar(index++, size.x * size.y);
    tex.pushTexture(index++, image, {});
    tex.pushBuffer(index++, buffer, 0, bufferSize);
    // texture pass
    dev.dispatchCompute(tex.program.material, tex.data, scratchBufferSize, 1);
  }

  // gpu pass
  auto     buff = ShaderMaterial(*bufferPass);
  uint32_t skip = 1;
  while (scratchBufferSize > 1)
  {
    buff.reset();
    uint32_t index = 0;
    buff.pushScalar(index++, block);
    buff.pushScalar(index++, scratchBufferSize);
    buff.pushScalar(index++, skip);
    buff.pushBuffer(index++, buffer, 0, bufferSize);
    scratchBufferSize = (scratchBufferSize + block - 1) / block;
    dev.barrier(GfxBarrierFlags::fStorageBuffer);
    dev.dispatchCompute(buff.program.material, buff.data, scratchBufferSize, 1);
    skip *= block;
  }

  float hrange[2] = {0.f, 0.f};
  dev.readBuffer(buffer, 0, std::span<ubyte_t>((ubyte_t*)hrange, sizeof(hrange)));
  dev.destroy(buffer);
  return vec2{hrange[0], hrange[1]};
}

} // namespace terra