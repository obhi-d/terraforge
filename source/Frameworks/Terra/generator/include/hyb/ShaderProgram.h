
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
  using Offset   = ShaderProgram::Offset;
  using Location = ShaderProgram::Location;

  Offset   bind_buffer(std::string_view, uint32_t nodeId);
  Offset   bind_uvec2(std::string_view, uint32_t nodeId);
  Offset   bind_ivec2(std::string_view, uint32_t nodeId);
  Offset   bind_vec2(std::string_view, uint32_t nodeId);
  Offset   bind_float(std::string_view, uint32_t nodeId);
  Offset   bind_int(std::string_view, uint32_t nodeId);
  Offset   bind_uint(std::string_view, uint32_t nodeId);
  Offset   bind_sampler2D(std::string_view, uint32_t nodeId);
  Location bind_output_texture(std::string_view, uint32_t nodeId);

  void push_input_modifier(std::string_view, uint32_t nodeId);
  void append_code(std::string_view, uint32_t nodeId);
  void append_code(std::string_view);

  std::string              code;
  std::vector<std::string> inputs;
  ShaderProgramPtr         program;
};

} // namespace terra