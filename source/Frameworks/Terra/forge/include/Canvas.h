
#pragma once

#include "Setup.h"

namespace terra
{

class Canvas
{
public:
  void color(ImageFormatEnum);
  void depth(ImageFormatEnum);
  void resize(glm::uvec2);
  void clear();

  GfxImage::handle get(uint32_t i) const
  {
    return images[i];
  }

  uvec2 getSize() const
  {
    return size;
  }

  void begin(bool reverseZ);
  void end();

private:
  void clearHandles();

  std::array<ImageFormatEnum, 8>  desc       = {};
  std::array<GfxImage::handle, 8> images     = {};
  ImageFormatEnum                 depthDesc  = {};
  GfxImage::handle                depthImage = {};
  GfxPass::handle                 pass       = {};
  glm::uvec2                      size       = {0, 0};
  uint32_t                        numImages  = 0;
};

} // namespace terra