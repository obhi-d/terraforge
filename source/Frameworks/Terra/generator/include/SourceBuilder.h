#pragma once
#include "RenderResource.h"
#include "ShaderOptions.h"
#include <acl/dynamic_array.hpp>
#include <sstream>

namespace terra
{
// Source builder is bindless shader builder
struct ShaderProgram
{
  std::vector<GfxParamLayout::Entry> entries;
  GfxParamLayout::UBOReflect         refl;
  GfxProgram::handle                 program;
  GfxParamLayout::handle             layout;
  uint32_t                           frame = 0;

  ShaderProgram()                              = default;
  ShaderProgram(ShaderProgram const&) noexcept = delete;
  ShaderProgram(ShaderProgram&&) noexcept      = default;
  ~ShaderProgram();

  ShaderProgram& operator=(ShaderProgram const&) noexcept = delete;
  ShaderProgram& operator=(ShaderProgram&&) noexcept;

  void touch();
};

using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;

struct ShaderMaterial
{
  ShaderMaterial(ShaderProgram const& prog) : program(prog), ubo(prog.refl.uboSize) {}

  inline void pushBuffer(GfxBuffer::handle buff, uint32_t offset, uint32_t size);
  inline void pushTexture(GfxImage::handle handle, GfxSampler::handle);
  inline void pushTexBuffer(GfxBuffer::handle handle, ImageFormatEnum format);
  inline void pushImage(GfxImage::handle handle, uint16_t layer, GfxAccess access, bool layered);
  template <typename T>
  inline void pushScalar(T const& value);
  inline void pushArray(std::span<ubyte_t const> data);
  inline void reset();

  inline GfxMaterial2 get() const
  {
    return GfxMaterial2(program.program, program.layout, bindings, ubo);
  }

  ShaderProgram const&        program;
  Blob                        bindings;
  acl::dynamic_array<ubyte_t> ubo;
  uint32_t                    curBindingIdx = 0;
  uint32_t                    curScalarIdx  = 0;
};

struct GpuPipeline
{
  std::vector<ShaderProgramPtr> passes;
};

using GpuPipelinePtr = std::shared_ptr<GpuPipeline>;
using GpuPipelineRef = std::weak_ptr<GpuPipeline>;

enum class SourceType
{
  eFullscreenGraphNode,
  eShaderProgram,
  ePostProcess,
  eComputeProgram
};

struct SourceBuilder
{
  virtual void             options(ShaderOptions option)                        = 0;
  virtual void             option(std::string_view)                             = 0;
  virtual void             pushExtension(std::string_view ext)                  = 0;
  virtual void             param(std::string_view name, DataFormat df)          = 0;
  virtual void             scalar(std::string_view name, DataFormat df)         = 0;
  virtual void             output(std::string_view name, DataFormat df)         = 0;
  virtual void             computeInput(std::string_view)                       = 0;
  virtual void             append(std::string_view)                             = 0;
  virtual void             call(std::string_view node, bool acceptInput = true) = 0;
  virtual ShaderProgramPtr finalize()                                           = 0;
};

struct SourceBuilderAdapter : SourceBuilder
{
  SourceBuilderAdapter(SourceType itype);

  void options(ShaderOptions option) final;
  void option(std::string_view name) final;
  void pushExtension(std::string_view ext) final;
  void param(std::string_view name, DataFormat df) final;
  void output(std::string_view name, DataFormat df) final;

  void sampleSSBO(std::string_view name, DataFormat df);
  void computeInput(std::string_view) final;
  void append(std::string_view) final;
  void call(std::string_view node, bool acceptInput);

  ShaderProgramPtr finalize();

  void               scalar(std::string_view name, DataFormat df) final;
  void               packUbo();
  void               packCommon(std::vector<std::string_view>&);
  GfxProgram::handle makeGpuNode(std::vector<std::string_view>&);
  GfxProgram::handle makePostProcess(std::vector<std::string_view>&);
  GfxProgram::handle makeShaderProgram(std::vector<std::string_view>&);
  GfxProgram::handle makeComputeProgram(std::vector<std::string_view>&);

  virtual void       sampleTexture(std::string_view name, DataFormat df)       = 0;
  virtual void       sampleImage(std::string_view name, DataFormat df)         = 0;
  virtual void       sampleTextureBuffer(std::string_view name, DataFormat df) = 0;
  inline std::string localName(std::string_view input)
  {
    return std::format("lp_{}", input);
  }

  std::vector<GfxParamLayout::Entry>    entries;
  std::vector<GfxParamLayout::UBOEntry> uboEntries;
  std::vector<std::string>              params;

  SourceType type        = SourceType::eFullscreenGraphNode;
  uint32_t   outputIdx   = 0;
  uint32_t   ssboBinding = 0;

  std::string regex;
  std::string optionHeader;
  std::string extensions;
  std::string resources;
  std::string includes;
  std::string functions;
  std::string input;
  std::string content;
  std::string ubo;

  uint32_t location = 0;
};

struct SourceBuilderBindless : SourceBuilderAdapter
{
  SourceBuilderBindless(SourceType t) : SourceBuilderAdapter(t)
  {
    // pushExtension("#extension GL_ARB_gpu_shader_int64 : require");
    pushExtension("#extension GL_ARB_bindless_texture : require");
  }

  void sampleTexture(std::string_view name, DataFormat df) final;
  void sampleImage(std::string_view name, DataFormat df) final;
  void sampleTextureBuffer(std::string_view name, DataFormat df) final;
};

struct SourceBuilderBindful : SourceBuilderAdapter
{
  SourceBuilderBindful(SourceType t) : SourceBuilderAdapter(t) {}
  void sampleTexture(std::string_view name, DataFormat df) final;
  void sampleImage(std::string_view name, DataFormat df) final;
  void sampleTextureBuffer(std::string_view name, DataFormat df) final;

  uint32_t texBinding   = 0;
  uint32_t imageBinding = 0;
};

inline void ShaderMaterial::pushBuffer(GfxBuffer::handle buff, uint32_t offset, uint32_t size)
{
  assert(program.entries[curBindingIdx].type == StorageBuffer::type);
  StorageBuffer ssbo;
  ssbo.buffer = buff;
  ssbo.offset = offset;
  ssbo.size   = size;
  bindings.push(ssbo);
  curBindingIdx++;
}

inline void ShaderMaterial::pushTexture(GfxImage::handle handle, GfxSampler::handle sampler)
{
  assert(std::find(SampledTexture::type.begin(), SampledTexture::type.end(), program.entries[curBindingIdx].type) !=
         SampledTexture::type.end());
  SampledTexture stex;
  stex.texture = handle;
  stex.sampler = sampler;
  bindings.push(stex);
  curBindingIdx++;
}

inline void ShaderMaterial::pushTexBuffer(GfxBuffer::handle handle, ImageFormatEnum format)
{
  assert(program.entries[curBindingIdx].type == TextureBuffer::type);
  TextureBuffer tbo;
  tbo.buffer = handle;
  tbo.format = format;
  bindings.push(tbo);
  curBindingIdx++;
}

inline void ShaderMaterial::pushImage(GfxImage::handle handle, uint16_t layer, GfxAccess access, bool layered)
{
  assert(program.entries[curBindingIdx].type == StorageImage::type);
  StorageImage image;
  image.texture = handle;
  image.layer   = layer;
  image.access  = access;
  image.layered = layered;
  bindings.push(image);
  curBindingIdx++;
}

template <typename T>
inline void ShaderMaterial::pushScalar(T const& value)
{
  if (program.refl.offsets[curScalarIdx].offset != 0xffffffff)
    std::memcpy(ubo.begin() + program.refl.offsets[curScalarIdx].offset, &value, sizeof(value));
  curScalarIdx++;
}

inline void ShaderMaterial::pushArray(std::span<ubyte_t const> data)
{
  auto     d         = program.refl.offsets[curScalarIdx];
  uint32_t remaining = 0;
  auto     src       = data.data();
  auto     end       = data.size() + src;
  auto     dst       = ubo.begin() + d.offset;
  while (src < end)
  {
    std::memcpy(dst, src, d.baseElementSize);
    src += d.baseElementSize;
    dst += d.arrayStride;
  }
  curScalarIdx++;
}

inline void ShaderMaterial::reset()
{
  curScalarIdx  = 0;
  curBindingIdx = 0;
  bindings.clear();
}

} // namespace terra