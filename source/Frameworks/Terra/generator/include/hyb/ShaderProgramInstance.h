
#pragma once

#include "Common.h"
#include "ParamHelper.h"
#include "ShaderOptions.h"
#include "SourceBuilder.h"
#include "hyb/HybridBuffer.h"

namespace terra
{
class HybridPipeline;

struct ShaderProgramInstance
{
  inline void pushValue(float value)
  {
    program.pushScalar(value);
  }

  inline void pushValue(int value)
  {
    program.pushScalar(value);
  }

  inline void pushValue(vec2 value)
  {
    program.pushScalar(value);
  }

  inline void pushValue(ivec2 value)
  {
    program.pushScalar(value);
  }

  void pushValue(Parameter const& value, DataTypeEnum type, DataTypeEnum subType);
  void pushValue(HybridBuffer::handle, DataFormat);
  void pushImage(GfxImage::handle, DataFormat);
  void pushBuffer(GfxBuffer::handle, uint32_t size, DataFormat);
  void pushOutput(HybridBuffer::handle, DataFormat, bool clear, vec4 clearVal);
  void run();

  ShaderProgramInstance(GfxState const& gfxstate, ShaderProgram const& iprogram, HybridPipeline& pipe)
      : pipeline(pipe), program(iprogram), state(gfxstate)
  {}

  std::array<GfxPass::Attachment, 8> outputs;
  GfxPass::Attachment                depth;

  GfxState const& state;
  HybridPipeline& pipeline;
  ShaderMaterial  program;

  uint32_t outputIdx = 0;
};

} // namespace terra