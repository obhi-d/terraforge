
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
  void pushValue(float);
  void pushValue(int);
  void pushValue(vec2);
  void pushValue(ivec2);
  void pushValue(ScalarValue value, DataTypeEnum type);
  void pushValue(HybridBuffer::handle, DataFormat);
  void pushOutput(HybridBuffer::handle, DataFormat);
  void run();

  ShaderProgramInstance(ShaderProgram const& iprogram, HybridPipeline& pipe) : pipeline(pipe), program(iprogram) {}
  HybridPipeline&      pipeline;
  ShaderProgram const& program;
  std::vector<ubyte_t> data;
  uint32_t             inputIndex  = 0;
  uint32_t             outputIndex = 0;
};

} // namespace terra