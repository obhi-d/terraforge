
#pragma once

#include "Common.h"

namespace terra
{
class HybridPipeline;
struct ShaderProgram
{
  virtual void setInput(int p, Source, , HybridPipeline&)    = 0;
  virtual void setInput(int p, ScalarValue, HybridPipeline&) = 0;
  virtual void setOutput(int p, Source, HybridPipeline&)     = 0;
};

struct GpuProgram : ShaderProgram
{
  void setInput(int p, Source, HybridPipeline&) override;
  void setInput(int p, ScalarValue, HybridPipeline&) override;
  void setOutput(int p, Source, HybridPipeline&) override;
};

} // namespace terra