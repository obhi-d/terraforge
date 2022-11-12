
#pragma once

namespace terra
{

struct MultiFractal
{
  float          amp;
  float          freq;
  hwybuffer_list outputs;
};

using IqfBm = MultiFractal;
} // namespace terra