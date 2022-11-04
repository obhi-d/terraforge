#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Pipeline_hwy.cpp"
#include <hwy/foreach_target.h>

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"

#include "Terra.h"
#include "Logger.h"
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;
using T      = float;

uint32_t lanes()
{
  const hn::ScalableTag<T> d;
  return (uint32_t)hn::Lanes(d);
}

void writeInputLine(float freq, uint32_t startx, uint32_t liney, uint32_t pitch, float* x, float* y)
{
  const hn::ScalableTag<T> d;
  auto                     line = hn::Iota(d, (float)startx);
  auto const               lanes = (uint32_t)hn::Lanes(d);
  auto const               vfreq = hn::Set(d, freq);
  auto const               vy = hn::Set(d, freq * float(liney));
  for (uint32_t i = 0; i < pitch; i += lanes)
  {
    hn::Store(hn::Mul(line, vfreq), d, x + i);
    hn::Store(vy, d, y + i);
    line += hn::Set(d, (float)lanes);
  }
}

vec2 getMinMax(float const* input, uint32_t pitch, uint32_t width, uint32_t height)
{
  const hn::ScalableTag<T> d;
  auto const               lanes = (uint32_t)hn::Lanes(d);

  auto const minv = hn::Set(d, std::numeric_limits<float>::max());
  auto const maxv = hn::Set(d, std::numeric_limits<float>::min());

  auto min = minv;
  auto max = maxv;
  for (uint32_t h = 0; h < height; h++)
  {
    auto line = input + pitch * h;
    for (uint32_t i = 0; i < pitch; i += lanes)
    {
      if (i + lanes <= width)
      {
        auto v = hn::Load(d, line + i);
        max = hn::Max(v, max);
        min = hn::Min(v, min);
      }
      else
      {
        auto m = hn::FirstN(d, i + lanes - width);
        max    = hn::Max(hn::IfThenElse(m, hn::Load(d, line + i), maxv), max);
        min    = hn::Min(hn::IfThenElse(m, hn::Load(d, line + i), minv), min);
      }
    }
  }

  vec2 value;
  value[0] = hn::GetLane(hn::MinOfLanes(d, min));
  value[1] = hn::GetLane(hn::MaxOfLanes(d, max));
  return value;
}

} // namespace terra::HWY_NAMESPACE

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{

HWY_EXPORT(lanes);
HWY_EXPORT(writeInputLine);
HWY_EXPORT(getMinMax);

uint32_t lanes()
{
  return HWY_DYNAMIC_DISPATCH(lanes)();
}

hwybuffer& Pipeline_hwy::getOutput(uint32_t thread, uint32_t lanes)
{
  auto& threadData = threadDatas[thread];
  if (threadData.outputs.empty())
    return pushOutput(thread, lanes);

  return threadData.outputs.back();
}

hwybuffer& Pipeline_hwy::pushOutput(uint32_t thread, uint32_t lanes)
{
  auto& threadData = threadDatas[thread];
  threadData.outputs.emplace_back(threadData.width, threadData.height, lanes);
  return threadData.outputs.back();
}

void Pipeline_hwy::popOutput(uint32_t thread)
{
  auto& threadData = threadDatas[thread];
  threadData.outputs.pop_back();
}


hwyvb& Pipeline_hwy::getInput(uint32_t thread, uint32_t lanes, bool populated)
{
  auto& threadData = threadDatas[thread];
  if (threadData.inputs.empty())
    return pushInput(thread, lanes, populated);

  return threadData.inputs.back();
}

hwyvb& Pipeline_hwy::pushInput(uint32_t thread, uint32_t lanes, bool populated)
{
  auto& threadData = threadDatas[thread];
  threadData.inputs.emplace_back(threadData.width, threadData.height, lanes);
  if (populated)
  {
    auto& inp = threadData.inputs.back();
    auto  x   = inp.x.data();
    auto  y   = inp.y.data();
    auto  xstart = threadData.params.startxy[0] - 1;
    auto  ystart = threadData.params.startxy[1] - 1;
    for (int i = 0; i < threadData.height; ++i)
    {
      HWY_DYNAMIC_DISPATCH(writeInputLine)(frequency(), xstart,
         ystart + i, inp.pitch(), x, y);
      x += inp.pitch();
      y += inp.pitch();
    }
  }
  return threadData.inputs.back();
}

void Pipeline_hwy::popInput(uint32_t thread)
{
  auto& threadData = threadDatas[thread];
  threadData.inputs.pop_back();
}

void Pipeline_hwy::pushTileTask(EnvParams const& envParams)
{
  // auto div = N * lanes();
  int32      nbWidth  = (envParams.tileRegion.size[0] + (N - 1)) / N;
  int32      nbHeight = (envParams.tileRegion.size[1] + (N - 1)) / N;
  threadDatas.reserve(nbHeight * nbWidth);
  for (int32_t y = 0; y < nbHeight; ++y)
  {
    for (int32_t x = 0; x < nbWidth; ++x)
    {
      auto nX           = x * N;
      auto nY           = y * N;
      threadDatas.emplace_back();
      auto& threadData  = threadDatas.back();
      threadData.params = envParams;
      threadData.params.startxy[0] += nX;
      threadData.params.startxy[1] += nY;
      threadData.params.region.offset[0] += nX;
      threadData.params.region.offset[1] += nY;
      threadData.params.tileRegion.offset[0] += nX;
      threadData.params.tileRegion.offset[1] += nY;

      int dx = std::min<int>(threadData.params.tileRegion.offset[0] + N,
                             envParams.tileRegion.offset[0] + envParams.tileRegion.size[0]) -
               threadData.params.tileRegion.offset[0];
      int dy = std::min<int>(threadData.params.tileRegion.offset[1] + N,
                             envParams.tileRegion.offset[1] + envParams.tileRegion.size[1]) -
               threadData.params.tileRegion.offset[1];
      if (dx > 0 && dy > 0)
      {
        threadData.params.tileRegion.size[0] = dx;
        threadData.params.tileRegion.size[1] = dy;
        threadData.params.region.size[0]     = dx;
        threadData.params.region.size[1]     = dy;
        threadData.width                     = dx + 2;
        threadData.height                    = dy + 2;
        threadData.thread                    = (uint32_t)threadDatas.size() - 1;
      }
      else
        threadDatas.pop_back();
    }
  }
}

void Pipeline_hwy::launch()
{
  get().pool().for_each(threadDatas.begin(), threadDatas.end(),
  [this](ThreadData& data)
  {
    NodeMeta_hwy::run(getActor(), *this, data.thread, lanes());
    if (!data.outputs.empty())
    {
      auto& back = data.outputs.back();
      data.minMax = HWY_DYNAMIC_DISPATCH(getMinMax)(back.data(), back.pitch(), back.width(), back.height());
    }
  }, waiters);
}

std::size_t Pipeline_hwy::hasResults()
{
  if (!threadDatas.empty())
  {
    auto const& ls = launchSize();
    return (size_t)ls[0] * (size_t)ls[1];
  }
  return 0;
}

void Pipeline_hwy::getResults(float* ready, uint32_t size, float& min, float& max)
{
  min = std::numeric_limits<float>::max();
  max = std::numeric_limits<float>::min();
  auto const& ls    = launchSize();
  auto const& lo    = launchOffset();
  // auto        ready = std::unique_ptr<float[]>(new float[ls[0] * ls[1]]);
  for (auto& td : threadDatas)
  {
    if (!td.outputs.empty())
    {
      uint32 x = td.params.region.offset[0];
      uint32 y = td.params.region.offset[1];
      auto& back = td.outputs.back();
      for (uint32 i = 1; i < back.height() - 1; ++i, ++y)
      {
        auto offset = ready + (y * ls[0]) + x;
        assert((y * ls[0]) + x + (td.params.region.size[0] * sizeof(float)) < size);
        std::memcpy(offset, back.data() + i * back.pitch() + 1, td.params.region.size[0] * sizeof(float));
      }

      min = std::min(min, td.minMax[0]);
      max = std::max(max, td.minMax[1]);
    }
  }

  threadDatas.clear();

  if (updateActor())
    launch();
}

void Pipeline_hwy::wait()
{
}

} // namespace terra
#endif
