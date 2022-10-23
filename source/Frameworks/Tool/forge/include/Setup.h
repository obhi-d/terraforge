#pragma once
#include "Common.h"
#include "Logger.h"
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>


namespace terra
{
struct AppSettings
{
  std::string name;
  std::string language         = "en-US";
  glm::ivec2  viewerSize       = glm::ivec2(1024, 768);
  glm::ivec2  viewerPos        = glm::ivec2(20, 20);
  std::string theme            = "themes/default.tns";
  int         glslVersion      = 130;
  bool        verbose          = true;
  bool        wasLoaded        = false;
  int         tileSize         = 119;
  int         numTilesPreviewX = 1;
  int         numTilesPreviewY = 1;
};


} // namespace terra