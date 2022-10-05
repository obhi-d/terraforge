#pragma once
#include <glm/glm.hpp>
#include "Logger.h"
#include "Common.h"
#include <string>
#include <cassert>

namespace terra
{
struct AppSettings
{
  std::string name;
  glm::ivec2  viewerSize = glm::ivec2(1024, 768);
  glm::ivec2  viewerPos = glm::ivec2(0, 0);
  std::string theme       = "default.tns";
  int         glslVersion = 130;
  bool        verbose     = true;
};
} // namespace terra