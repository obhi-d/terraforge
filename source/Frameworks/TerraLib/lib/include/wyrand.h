#pragma once
#include <cstdint>

static inline uint64_t wyrand(uint64_t* seed)
{
  *seed += 0xa0761d6478bd642full;
  uint64_t see1 = *seed ^ 0xe7037ed1a0b428dbull;
  see1 *= (see1 >> 32) | (see1 << 32);
  return (*seed * ((*seed >> 32) | (*seed << 32))) ^ ((see1 >> 32) | (see1 << 32));
}
