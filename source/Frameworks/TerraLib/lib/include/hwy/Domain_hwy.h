
#pragma once

#include "hwy/Buffer_hwy.h"

namespace terra
{

struct DomainFractal
{
  float          amp;
  float          freq;
  int32_t        seed;
  hwyvb_list     inputs;
};

} // namespace terra