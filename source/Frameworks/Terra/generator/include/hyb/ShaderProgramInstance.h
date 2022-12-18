
#pragma once

#include "Common.h"
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
    program.pushScalar(index++, value);
  }

  inline void pushValue(int value)
  {
    program.pushScalar(index++, value);
  }

  inline void pushValue(vec2 value)
  {
    program.pushScalar(index++, value);
  }

  inline void pushValue(ivec2 value)
  {
    program.pushScalar(index++, value);
  }

  void pushValue(ScalarValue value, DataTypeEnum type);
  void pushValue(HybridBuffer::handle, DataFormat);
  void pushOutput(HybridBuffer::handle, bool clear, vec4 clearVal);
  void run();

  ShaderProgramInstance(ShaderProgram const& iprogram, HybridPipeline& pipe) : pipeline(pipe), program(iprogram) {}
  HybridPipeline& pipeline;
  ShaderMaterial  program;
  uint32_t        index = 0;
};

} // namespace terra