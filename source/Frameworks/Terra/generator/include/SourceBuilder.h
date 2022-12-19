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
  acl::dynamic_array<GfxBindType> bindings;

  GfxMaterial2 material;
  uint32_t     outputCount = 0;
  uint32_t     frame       = 0;

  ShaderProgram()                              = default;
  ShaderProgram(ShaderProgram const&) noexcept = default;
  ShaderProgram(ShaderProgram&&) noexcept      = default;
  ~ShaderProgram();

  ShaderProgram& operator=(ShaderProgram const&) noexcept;
  ShaderProgram& operator=(ShaderProgram&&) noexcept;

  void touch();
};

struct ShaderMaterial
{
  ShaderMaterial(ShaderProgram const& prog) : program(prog) {}

  inline void pushBuffer(uint32_t index, GfxBuffer::handle buff, uint32_t offset, uint32_t size);
  inline void pushTexture(uint32_t index, GfxImage::handle handle, GfxSampler::handle);
  inline void pushTexBuffer(uint32_t index, GfxBuffer::handle handle, ImageFormatEnum format);
  inline void pushImage(uint32_t index, GfxImage::handle handle, uint16_t layer, GfxAccess access, bool layered);
  inline void pushScalar(uint32_t index, int value);
  inline void pushScalar(uint32_t index, uint32_t value);
  inline void pushScalar(uint32_t index, ivec2 value);
  inline void pushScalar(uint32_t index, uvec2 value);
  inline void pushScalar(uint32_t index, float value);
  inline void pushScalar(uint32_t index, vec2 value);
  inline void pushScalar(uint32_t index, vec3 value);
  inline void pushScalar(uint32_t index, vec4 value);
  inline void pushScalar(uint32_t index, mat4 value);
  inline void pushOutput(uint32_t index, GfxImage::handle handle, bool clear = false, vec4 clearVal = vec4(0.f));
  inline void reset();

  ShaderProgram const& program;
  Blob                 data;
};

struct GpuPipeline
{
  std::vector<ShaderProgram> passes;
};

using GpuPipelinePtr = std::shared_ptr<GpuPipeline>;
using GpuPipelineRef = std::weak_ptr<GpuPipeline>;

enum class SourceType
{
  eFullscreenGraphNode,
  eShaderProgram,
  ePostProcess
};

struct SourceBuilder
{
  virtual void          pushOptions(ShaderOptions option)                    = 0;
  virtual void          pushExtension(std::string_view ext)                  = 0;
  virtual void          sampleParam(std::string_view name, DataFormat df)    = 0;
  virtual void          sampleScalar(std::string_view name, DataFormat df)   = 0;
  virtual void          computeParam(std::string_view name, DataFormat df)   = 0;
  virtual void          writeOutput(std::string_view name, DataFormat df)    = 0;
  virtual void          computeInput(std::string_view)                       = 0;
  virtual void          append(std::string_view)                             = 0;
  virtual void          pushScope(std::string_view)                          = 0;
  virtual void          call(std::string_view node, bool acceptInput = true) = 0;
  virtual ShaderProgram finalize()                                           = 0;
};

struct SourceBuilderAdapter : SourceBuilder
{
  SourceBuilderAdapter(SourceType itype) : type(itype) {}

  void pushOptions(ShaderOptions option) final;
  void pushExtension(std::string_view ext) final;
  void sampleParam(std::string_view name, DataFormat df) final;
  void sampleScalar(std::string_view name, DataFormat df) final;
  void computeParam(std::string_view name, DataFormat df) final;
  void writeOutput(std::string_view name, DataFormat df);

  void sampleSSBO(std::string_view name, DataFormat df);
  void computeInput(std::string_view) final;
  void append(std::string_view) final;
  void call(std::string_view node, bool acceptInput);

  ShaderProgram finalize();

  void               packCommon(std::vector<std::string_view>&);
  GfxProgram::handle makeGpuNode(std::vector<std::string_view>&);
  GfxProgram::handle makePostProcess(std::vector<std::string_view>&);
  GfxProgram::handle makeShaderProgram(std::vector<std::string_view>&);

  virtual void sampleTexture(std::string_view name, DataFormat df)       = 0;
  virtual void sampleImage(std::string_view name, DataFormat df)         = 0;
  virtual void sampleTextureBuffer(std::string_view name, DataFormat df) = 0;

  std::string format(std::string data);

  uint32_t swapId(uint32_t with)
  {
    std::swap(id, with);
    return with;
  }

  std::vector<GfxParamLayout::Entry>  entries;
  std::vector<GfxParamLayout::Output> output;
  std::vector<std::string>            params;

  SourceType type        = SourceType::eFullscreenGraphNode;
  uint32_t   outputIdx   = 0;
  uint32_t   ssboBinding = 0;
  uint32_t   id          = 0;

  std::string options;
  std::string extensions;
  std::string resources;
  std::string ubo;
  std::string includes;
  std::string functions;
  std::string input;
  std::string content;
};

struct SourceBuilderBindless : SourceBuilderAdapter
{
  SourceBuilderBindless(SourceType t) : SourceBuilderAdapter(t) {}
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

inline void ShaderMaterial::pushBuffer(uint32_t index, GfxBuffer::handle buff, uint32_t offset, uint32_t size)
{
  assert(program.bindings[index] == StorageBuffer::type);
  StorageBuffer ssbo;
  ssbo.buffer = buff;
  ssbo.offset = offset;
  ssbo.size   = size;
  data.push(ssbo);
  index++;
}

inline void ShaderMaterial::pushTexture(uint32_t index, GfxImage::handle handle, GfxSampler::handle sampler)
{
  assert(program.bindings[index] == SampledTexture::type);
  SampledTexture stex;
  stex.texture = handle;
  stex.sampler = sampler;
  data.push(stex);
  index++;
}

inline void ShaderMaterial::pushTexBuffer(uint32_t index, GfxBuffer::handle handle, ImageFormatEnum format)
{
  assert(program.bindings[index] == TextureBuffer::type);
  TextureBuffer tbo;
  tbo.buffer = handle;
  tbo.format = format;
  data.push(tbo);
  index++;
}

inline void ShaderMaterial::pushImage(uint32_t index, GfxImage::handle handle, uint16_t layer, GfxAccess access,
                                      bool layered)
{
  assert(program.bindings[index] == StorageImage::type);
  StorageImage image;
  image.texture = handle;
  image.layer   = layer;
  image.access  = access;
  image.layered = layered;
  data.push(image);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, int value)
{
  assert(program.bindings[index] == GfxBindType::eInt);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, uint32_t value)
{
  assert(program.bindings[index] == GfxBindType::eUint);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, ivec2 value)
{
  assert(program.bindings[index] == GfxBindType::eInt2);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, uvec2 value)
{
  assert(program.bindings[index] == GfxBindType::eUint2);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, float value)
{
  assert(program.bindings[index] == GfxBindType::eFloat);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, vec2 value)
{
  assert(program.bindings[index] == GfxBindType::eFloat2);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, vec3 value)
{
  assert(program.bindings[index] == GfxBindType::eFloat3);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, vec4 value)
{
  assert(program.bindings[index] == GfxBindType::eFloat4);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushScalar(uint32_t index, mat4 value)
{
  assert(program.bindings[index] == GfxBindType::eMat4);
  data.push(value);
  index++;
}

inline void ShaderMaterial::pushOutput(uint32_t index, GfxImage::handle handle, bool clear, vec4 clearVal)
{
  assert(program.bindings.size() >= index && index < (program.bindings.size() + program.outputCount));
  TextureOutput texture;
  texture.image      = handle;
  texture.clear      = clear;
  texture.clearValue = clearVal;
  data.push(texture);
  index++;
}

inline void ShaderMaterial::reset()
{
  data.clear();
}

} // namespace terra