
#pragma once

#include "Common.h"

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

struct GpuProgramBuilder
{
  struct Binding
  {
    uint16_t type  = 0;
    uint16_t index = 0;
  };

  using Offset   = ShaderProgram::Offset;
  using Location = ShaderProgram::Location;

  Binding  bind_buffer(std::string_view, uint32_t nodeId);
  Binding  bind_ivec2(std::string_view, uint32_t nodeId);
  Binding  bind_vec2(std::string_view, uint32_t nodeId);
  Binding  bind_float(std::string_view, uint32_t nodeId);
  Binding  bind_int(std::string_view, uint32_t nodeId);
  Binding  bind_sampler2D(std::string_view, uint32_t nodeId);
  Location bind_output_texture(std::string_view, uint32_t nodeId);

  void push_input_modifier(std::string_view, uint32_t nodeId);
  void append_code(std::string_view, uint32_t nodeId);
  void append_code(std::string_view);

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
};

} // namespace terra