#pragma once
#include "Common.h"
#include "Logger.h"
#include <cassert>
#include <cstdint>
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
  }                                                                                                                    \
  inline Enum& operator&=(Enum& a, Enum b)                                                                             \
  {                                                                                                                    \
    return (Enum&)((uint32_t&)a &= (uint32_t)b);                                                                       \
  }                                                                                                                    \
  inline Enum& operator|=(Enum& a, Enum b)                                                                             \
  {                                                                                                                    \
    return (Enum&)((uint32_t&)a |= (uint32_t)b);                                                                       \
  }                                                                                                                    \
  inline bool operator!(Enum a)                                                                                        \
  {                                                                                                                    \
    return ((uint32_t)a != 0);                                                                                         \
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
  bool        wasLoaded   = false;
};

inline uintptr_t pack(uint32_t first, uint32_t sec)
{
  return (uintptr_t)first << 32ull | (uintptr_t)sec;
}

using uintpair = std::pair<uint32_t, uint32_t>;
inline uintpair unpack(uintptr_t v)
{
  return std::make_pair<uint32_t, uint32_t>(static_cast<uint32_t>(v >> 32ull), static_cast<uint32_t>(v & 0xffffffff));
}

} // namespace terra