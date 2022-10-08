#pragma once
#include "Common.h"
#include "Logger.h"
#include <cassert>
#include <glm/glm.hpp>
#include <string>

#define ENUM_FLAGS(Enum)                                                                                               \
  inline Enum operator|(Enum a, Enum b)                                                                                \
  {                                                                                                                    \
    return (Enum)((uint32_t)a | (uint32_t)b);                                                                          \
  }                                                                                                                    \
  inline bool operator&(Enum a, Enum b)                                                                                \
  {                                                                                                                    \
    return ((uint32_t)a & (uint32_t)b) != 0;                                                                           \
  }

namespace terra
{
struct AppSettings
{
  std::string name;
  std::string language    = "en-US";
  glm::ivec2  viewerSize  = glm::ivec2(1024, 768);
  glm::ivec2  viewerPos   = glm::ivec2(20, 20);
  std::string theme       = "themes/default.tns";
  int         glslVersion = 130;
  bool        verbose     = true;
};
} // namespace terra