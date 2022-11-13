
#pragma once

#include "hwy/Buffer_hwy.h"

namespace terra
{

struct DomainFractal
{
  float      amp;
  float      freq;
  hwyvb_list inputs;
};

} // namespace terra