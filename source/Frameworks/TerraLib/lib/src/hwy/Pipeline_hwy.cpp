
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Pipeline_hwy.cpp"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"

#include "Terra.h"

#include <hwy/foreach_target.h>
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

} // namespace terra::HWY_NAMESPACE

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{

HWY_EXPORT(lanes);
HWY_EXPORT(writeInputLine);

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
  if (threadData.outputs.empty())
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
    auto  xstart = threadData.params.tileOffset[0] + threadData.params.offset[0] - 1;
    auto  ystart = threadData.params.tileOffset[1] + threadData.params.offset[1] - 1;
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
  int32 nbWidth = (envParams.size[0] + (N - 1)) / N;
  int32 nbHeight = (envParams.size[1] + (N - 1)) / N;
  ThreadData threadData;
  threadData.params = envParams;
  for (int32_t y = 0; y < nbHeight; ++y)
  {
    for (int32_t x = 0; x < nbWidth; ++x)
    {
      threadData.params.offset[0] += x * N;
      threadData.params.offset[1] += y * N;
      int dx = (int)std::min<int>(threadData.params.offset[0] + N, envParams.offset[0] + envParams.size[0]) -
               (int)threadData.params.offset[0];
      int dy = (int)std::min<int>(threadData.params.offset[1] + N, envParams.offset[1] + envParams.size[1]) -
               (int)threadData.params.offset[1];
      if (dx > 0 && dy > 0)
      {
        threadData.params.size[0] = dx;
        threadData.params.size[1] = dy;
        threadData.width          = dx + 2;
        threadData.height         = dy + 2;

        threadDatas.push_back(threadData);
      }
    }
  }
}

void Pipeline_hwy::launch() 
{
  semaphore.acquire();
  finished = (uint32_t)threadDatas.size();
  for (uint32_t id = 0; id < (uint32_t)threadDatas.size(); ++id)
  {
    get().pool().add(
      [id, this]()
      {
        NodeMeta_hwy::run(getActor(), *this, id, lanes());
        if(finished.fetch_sub(1) == 1)
          semaphore.release();

      });
  }
}

std::unique_ptr<float[]> Pipeline_hwy::getResults() 
{
  if (finished.load() == 0)
  {
    auto const& ls    = launchSize();
    auto const& lo    = launchOffset();
    auto        ready = std::unique_ptr<float[]>(new float[ls[0] * ls[1]]);
    for (auto& td : threadDatas)
    {
      if (!td.outputs.empty())
      {
        auto& back = td.outputs.back();
        auto& buffer = td.outputs.back();
        for (uint32 i = 0; i < buffer.height(); ++i)
        {
          uint32 x = td.params.tileOffset[0] + td.params.offset[0] - lo[0];
          uint32 y = td.params.tileOffset[1] + td.params.offset[1] - lo[1];
          auto offset = ready.get() + (y * ls[0]) + x;
          std::memcpy(offset, back.data() + 1, td.params.size[0] * sizeof(float));
        }
      }
    }
    return ready;
  }
  return nullptr;
}

} // namespace terra
#endif
