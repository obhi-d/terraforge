#pragma once
#include <cstdint>

static inline uint64_t wyrand(uint64_t* seed)
{
  *seed += 0xa0761d6478bd642full;
  uint64_t see1 = *seed ^ 0xe7037ed1a0b428dbull;
  see1 *= (see1 >> 32) | (see1 << 32);
  return (*seed * ((*seed >> 32) | (*seed << 32))) ^ ((see1 >> 32) | (see1 << 32));
}

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
