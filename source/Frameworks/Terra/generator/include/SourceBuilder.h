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

  GfxProgram::handle     program;
  GfxParamLayout::handle layout;
  uint32_t               outputCount = 0;
  uint32_t               frame       = 0;

  ShaderProgram()                              = default;
  ShaderProgram(ShaderProgram const&) noexcept = default;
  ShaderProgram(ShaderProgram&&) noexcept      = default;
  ~ShaderProgram();

  ShaderProgram& operator=(ShaderProgram const&) noexcept = default;
  ShaderProgram& operator=(ShaderProgram&&) noexcept      = default;

  void touch();
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
  virtual uint32_t      swapId(uint32_t with)                                = 0;
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
} // namespace terra