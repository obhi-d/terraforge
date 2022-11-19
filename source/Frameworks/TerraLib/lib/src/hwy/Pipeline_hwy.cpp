#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Pipeline_hwy.cpp"
#include <hwy/foreach_target.h>

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"

#include "Logger.h"
#include "Terra.h"
#include "wyrand.h"
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
  auto                     line  = hn::Iota(d, (float)startx);
  auto const               lanes = (uint32_t)hn::Lanes(d);
  auto const               vfreq = hn::Set(d, freq);
  auto const               vy    = hn::Set(d, freq * float(liney));
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

  constexpr auto min = -std::numeric_limits<float>::infinity();
  constexpr auto max = std::numeric_limits<float>::infinity();

  auto minv = hn::Set(d, max);
  auto maxv = hn::Set(d, min);

  for (uint32_t h = 0; h < height; h++)
  {
    auto line = input + pitch * h;
    for (uint32_t i = 0; i < pitch; i += lanes)
    {
      if (i + lanes <= width)
      {
        auto v = hn::Load(d, line + i);
        maxv   = hn::Max(v, maxv);
        minv   = hn::Min(v, minv);
      }
      else
      {
        auto m = hn::FirstN(d, i + lanes - width);
        maxv   = hn::Max(hn::IfThenElse(m, hn::Load(d, line + i), maxv), maxv);
        minv   = hn::Min(hn::IfThenElse(m, hn::Load(d, line + i), minv), minv);
      }
    }
  }

  vec2 value;
  value[0] = hn::GetLane(hn::MinOfLanes(d, minv));
  value[1] = hn::GetLane(hn::MaxOfLanes(d, maxv));
  return value;
}

} // namespace terra::HWY_NAMESPACE

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{

std::array<std::array<int32_t, 4>, 64> PermuatationConstants::simplexlut = {
  {{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 0, 0, 0}, {0, 2, 3, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 2, 3, 0},
   {0, 2, 1, 3}, {0, 0, 0, 0}, {0, 3, 1, 2}, {0, 3, 2, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 3, 2, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {1, 2, 0, 3}, {0, 0, 0, 0}, {1, 3, 0, 2}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
   {1, 0, 2, 3}, {1, 0, 3, 2}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2, 0, 3, 1}, {0, 0, 0, 0}, {2, 1, 3, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {2, 0, 1, 3}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {3, 0, 1, 2}, {3, 0, 2, 1}, {0, 0, 0, 0}, {3, 1, 2, 0},
   {2, 1, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {3, 1, 0, 2}, {0, 0, 0, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}}};

constexpr float                      sqrt2by3                      = 0.81649658f; // std ::sqrt(2.f) / std::sqrt(3.f);
std::array<std::array<float, 3>, 16> PermuatationConstants::grad3u = {{{1.0f, 0.0f, 1.0f},
                                                                       {0.0f, 1.0f, 1.0f}, // 12 cube edges
                                                                       {-1.0f, 0.0f, 1.0f},
                                                                       {0.0f, -1.0f, 1.0f},
                                                                       {1.0f, 0.0f, -1.0f},
                                                                       {0.0f, 1.0f, -1.0f},
                                                                       {-1.0f, 0.0f, -1.0f},
                                                                       {0.0f, -1.0f, -1.0f},
                                                                       {sqrt2by3, sqrt2by3, sqrt2by3},
                                                                       {-sqrt2by3, sqrt2by3, -sqrt2by3},
                                                                       {-sqrt2by3, -sqrt2by3, sqrt2by3},
                                                                       {sqrt2by3, -sqrt2by3, -sqrt2by3},
                                                                       {-sqrt2by3, sqrt2by3, sqrt2by3},
                                                                       {sqrt2by3, -sqrt2by3, sqrt2by3},
                                                                       {sqrt2by3, -sqrt2by3, -sqrt2by3},
                                                                       {-sqrt2by3, sqrt2by3, -sqrt2by3}}};

std::array<std::array<float, 3>, 16> PermuatationConstants::grad3v = {{{-sqrt2by3, sqrt2by3, sqrt2by3},
                                                                       {-sqrt2by3, -sqrt2by3, sqrt2by3},
                                                                       {sqrt2by3, -sqrt2by3, sqrt2by3},
                                                                       {sqrt2by3, sqrt2by3, sqrt2by3},
                                                                       {-sqrt2by3, -sqrt2by3, -sqrt2by3},
                                                                       {sqrt2by3, -sqrt2by3, -sqrt2by3},
                                                                       {sqrt2by3, sqrt2by3, -sqrt2by3},
                                                                       {-sqrt2by3, sqrt2by3, -sqrt2by3},
                                                                       {1.0f, -1.0f, 0.0f},
                                                                       {1.0f, 1.0f, 0.0f},
                                                                       {-1.0f, 1.0f, 0.0f},
                                                                       {-1.0f, -1.0f, 0.0f},
                                                                       {1.0f, 0.0f, 1.0f},
                                                                       {-1.0f, 0.0f, 1.0f}, // 4 repeats to make 16
                                                                       {0.0f, 1.0f, -1.0f},
                                                                       {0.0f, -1.0f, -1.0f}}};

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
    auto& inp    = threadData.inputs.back();
    auto  x      = inp.x.data();
    auto  y      = inp.y.data();
    auto  xstart = threadData.params.startxy[0] - 1;
    auto  ystart = threadData.params.startxy[1] - 1;
    for (int i = 0; i < threadData.height; ++i)
    {
      HWY_DYNAMIC_DISPATCH(writeInputLine)(frequency(thread), xstart, ystart + i, inp.pitch(), x, y);
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

  tileDatas.emplace_back();
  TileData& data   = tileDatas.back();
  data.params      = envParams;
  data.threadStart = (uint32)threadDatas.size();
  // auto div = N * lanes();
  constexpr bool SingleThreaded = false;
  if constexpr (SingleThreaded)
  {
    threadDatas.emplace_back();
    auto& threadData           = threadDatas.back();
    threadData.params          = envParams;
    threadData.width           = threadData.params.tileRegion.size[0];
    threadData.height          = threadData.params.tileRegion.size[1];
    threadData.thread          = (uint32_t)threadDatas.size() - 1;
    threadData.uv.recipSize[0] = 1.f / (envParams.frequency * (float)envParams.tileSize[0]);
    threadData.uv.recipSize[1] = 1.f / (envParams.frequency * (float)envParams.tileSize[1]);
    threadData.uv.offset[0]    = envParams.startxy[0] * envParams.frequency;
    threadData.uv.offset[1]    = envParams.startxy[1] * envParams.frequency;
    threadData.tileIdx         = (uint32)tileDatas.size() - 1;
  }
  else
  {
    int32 nbWidth  = (envParams.tileRegion.size[0] + (N - 1)) / N;
    int32 nbHeight = (envParams.tileRegion.size[1] + (N - 1)) / N;
    threadDatas.reserve(nbHeight * nbWidth);
    for (int32_t y = 0; y < nbHeight; ++y)
    {
      for (int32_t x = 0; x < nbWidth; ++x)
      {
        auto nX = x * N;
        auto nY = y * N;
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
          threadData.width                     = dx;
          threadData.height                    = dy;
          threadData.thread                    = (uint32_t)threadDatas.size() - 1;
          threadData.uv.recipSize[0]           = 1.f / (envParams.frequency * (float)envParams.tileSize[0]);
          threadData.uv.recipSize[1]           = 1.f / (envParams.frequency * (float)envParams.tileSize[1]);
          threadData.uv.offset[0]              = threadData.params.startxy[0] * envParams.frequency;
          threadData.uv.offset[1]              = threadData.params.startxy[1] * envParams.frequency;
          threadData.tileIdx                   = (uint32)tileDatas.size() - 1;
        }
        else
          threadDatas.pop_back();
      }
    }
  }

  data.threadCount = (uint32)threadDatas.size() - data.threadStart;
  data.buffer.resize(envParams.tileSize[0] * envParams.tileSize[1]);

  if (envParams.seed != (int)constants.seed)
  {
    constants.seed = (uint64_t)envParams.seed;
    for (size_t i = 0; i < 256; ++i)
      constants.perm[i] = (int32_t)wyrand(&constants.seed);
  }
}

void Pipeline_hwy::launch()
{
  if (threadDatas.empty())
    return;
  reissueNode = {};
  if (iteration == 0)
    DataSource::prepareGeneration(getActor(), *this);
  onLaunch();
  DataSource::beginIteration(getActor(), *this);
  get().pool().for_each(
    threadDatas.begin(), threadDatas.end(),
    [this](ThreadData& data)
    {
      NodeMeta_hwy::run(getActor(), *this, data.thread, lanes());
      if (!data.outputs.empty())
      {
        auto& back  = data.outputs.back();
        data.minMax = HWY_DYNAMIC_DISPATCH(getMinMax)(back.data(), back.pitch(), back.width(), back.height());

        uint32 x = data.params.region.offset[0];
        uint32 y = data.params.region.offset[1];
        for (uint32 i = 0; i < back.height(); ++i, ++y)
        {
          auto offset = tileDatas[data.tileIdx].buffer.data() + (y * params().tileSize[0]) + x;
          assert((y * params().tileSize[0]) + x + (data.params.region.size[0]) <=
                 tileDatas[data.tileIdx].buffer.size());
          std::memcpy(offset, back.data() + i * back.pitch(), data.params.region.size[0] * sizeof(float));
        }
      }
    });
  DataSource::endIteration(getActor(), *this);
  if (hasMoreIterations())
    iteration++;
}

std::size_t Pipeline_hwy::hasResults()
{
  if (!threadDatas.empty() && !threadDatas[0].outputs.empty())
  {
    auto const& ls = launchSize();
    return (size_t)ls[0] * (size_t)ls[1];
  }
  return 0;
}

void Pipeline_hwy::getResults(float* ready, size_t nbFloats, float& min, float& max)
{
  min            = std::numeric_limits<float>::infinity();
  max            = -std::numeric_limits<float>::infinity();
  auto const& ls = launchSize();
  auto const& lo = launchOffset();
  auto const& ts = params().tileSize;
  // auto        ready = std::unique_ptr<float[]>(new float[ls[0] * ls[1]]);
  if (tileDatas.size() == 1)
  {
    assert(nbFloats == tileDatas[0].buffer.size());
    std::memcpy(ready, tileDatas[0].buffer.data(), nbFloats * sizeof(float));
  }
  else
  {
    for (auto& td : tileDatas)
    {
      if (!td.buffer.empty())
      {
        uint32 x   = td.params.region.offset[0];
        uint32 y   = td.params.region.offset[1];
        auto   src = td.buffer.data();
        for (uint32 i = 0; i < (uint32)ts[1]; ++i, ++y)
        {
          auto offset = ready + (y * ls[0]) + x;
          assert((y * ls[0]) + x + (td.params.region.size[0]) < nbFloats);
          std::memcpy(offset, src + i * ts[0], td.params.region.size[0] * sizeof(float));
        }
      }
    }
  }

  for (auto& td : threadDatas)
  {
    min = std::min(min, td.minMax[0]);
    max = std::max(max, td.minMax[1]);
  }

  if (hasMoreIterations())
    launch();
  else
    threadDatas.clear();
}

void Pipeline_hwy::wait() {}
void Pipeline_hwy::cleanup()
{
  threadDatas.clear();
  tileDatas.clear();
  cacheData.clear();
  Pipeline::cleanup();
}

} // namespace terra
#endif
