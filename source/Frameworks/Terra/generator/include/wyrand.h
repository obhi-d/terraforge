#pragma once
#include "wyhash.h"
#include <cstdint>
// range 0 to 1
static inline float wyunorm(uint64_t* seed)
{
  constexpr float scale = 1.f / 100000.f;
  return (float)(wyrand(seed) % 100000) * scale;
}

// range -1 to 1
static inline float wysnorm(uint64_t* seed)
{
  return (wyunorm(seed) - 0.5f) * 2.f;
}
