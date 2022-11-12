
#pragma once

#include "hwy/Buffer_hwy.h"

namespace terra
{

struct MultiFractal
{
  float          amp;
  float          freq;
  hwybuffer_list outputs;
};

struct IqfBm
{
  float          amp;
  float          freq;
  hwybuffer_list sum;
  hwybuffer_list dx;
};

} // namespace terra