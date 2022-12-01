
#pragma once

#include "Common.h"
#include "ShaderOptions.h"
#include "hyb/HybridBuffer.h"

namespace terra
{
class HybridPipeline;

struct ShaderProgram
{
  struct Offset
  {
    uint32_t offset = 0;
  };

  struct Location
  {
    uint32_t binding = 0;
  };

  GfxProgram::handle  program;
  uint32_t            bufferSize  = 0;
  uint32_t            outputCount = 0;
  uint32_t            lastUsage   = 0;
  std::vector<Offset> bindOffsets;

  void touch();
};

using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;
using ShaderProgramRef = std::weak_ptr<ShaderProgram>;

struct ShaderProgramInstance
{

  void pushValue(float);
  void pushValue(vec2);
  void pushValue(int);
  void pushValue(ivec2);
  void pushOutput(HybridBuffer::handle);
  void run();

  ShaderProgramInstance(ShaderProgramPtr iprogram) : program(iprogram) {}
  ShaderProgramPtr     program;
  std::vector<ubyte_t> data;
  uint32_t             inputIndex  = 0;
  uint32_t             outputIndex = 0;
};

class GpuProgramBuilder
{
public:
  GpuProgramBuilder(uint32_t i) : id(i) {}

  using Offset   = ShaderProgram::Offset;
  using Location = ShaderProgram::Location;

  void push_extension(std::string_view ext);
  void sample_param(std::string_view name, DataType primary, DataType secondary, uint32_t idx);
  void compute_param(std::string_view name, DataType primary, DataType secondary, uint32_t idx);
  void set_param(std::string_view name, DataType primary);

  uint32 swap_id(uint32_t i);
  void   push_options(ShaderOptions option);
  void   bind_buffer(std::string_view);
  void   bind_ivec2(std::string_view);
  void   bind_vec2(std::string_view);
  void   bind_float(std::string_view);
  void   bind_int(std::string_view);
  void   bind_sampler2D(std::string_view);
  void   bind_output_texture(std::string_view);

  void push_input_modifier(std::string_view);
  void append_code(std::string_view);
  void append_code(std::string_view);

  ShaderProgramPtr finalize();

private:
  uint16_t bufferCount    = 0;
  uint16_t ivec2Count     = 0;
  uint16_t vec2Count      = 0;
  uint16_t floatCount     = 0;
  uint16_t intCount       = 0;
  uint16_t sampler2DCount = 0;
  uint16_t outputCount    = 0;

  std::string              code;
  std::vector<std::string> inputs;
  std::vector<Offset>      bindOffsets;
  uint32_t                 bufferSize = 0;
  uint32_t                 id         = 0;
};

} // namespace terra