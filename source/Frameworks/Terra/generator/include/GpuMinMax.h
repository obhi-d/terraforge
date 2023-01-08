
#pragma once

#include "SourceBuilder.h"

namespace terra
{

class GpuMinMax
{
public:
  static void buildProgram();
  static void destroy();

  static vec2 execute(GfxImage::handle image, glm::uvec2 size, uint32_t block = 16);

private:
  static ShaderProgramPtr texturePass;
  static ShaderProgramPtr bufferPass;
  static ShaderProgramPtr normalizePass;
};

} // namespace terra