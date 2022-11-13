#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Domain_hwy.cpp"

#include "Common.h"
#include "Node.h"
#include <hwy/foreach_target.h>

#include "hwy/Domain_hwy.h"
#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/Utility_hwy.h"

#include "Terra.h"

HWY_BEFORE_NAMESPACE();
namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

void domainRotate(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  modifyDomain(node, pipe, threadGroupId);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto rad        = radians(std::get<ScalarValue>(node.param(1)).value);
  auto cos        = hn::Set(vtag, std::cos(rad));
  auto sin        = hn::Set(vtag, std::sin(rad));

  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto nx = hn::MulSub(cos, x, sin * y);
    auto ny = hn::MulAdd(sin, x, cos * y);

    hn::Store(nx, vtag, inp_x_data + ii);
    hn::Store(ny, vtag, inp_y_data + ii);
  }

  finish(node, pipe, threadGroupId);
}

void domainScaleOffset(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  modifyDomain(node, pipe, threadGroupId);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto sx         = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[0]);
  auto sy         = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[1]);
  auto ox         = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value2[0]);
  auto oy         = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value2[1]);

  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto nx = hn::MulAdd(sx, x, ox);
    auto ny = hn::MulAdd(sy, y, oy);

    hn::Store(nx, vtag, inp_x_data + ii);
    hn::Store(ny, vtag, inp_y_data + ii);
  }

  finish(node, pipe, threadGroupId);
}

void domainWarp(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& data = pipe.getCacheData<DomainFractal>(node.getSelf());
  auto& iof  = pipe.getInput(threadGroupId, lanes, true);
  auto  seed = hn::Set(itag, pipe.seed(threadGroupId));

  auto& sum       = data.inputs[threadGroupId];
  auto  iteration = pipe.getIteration();

  if (pipe.getIteration() == 0)
  {
    sum.x.fill(iof.x.width(), iof.x.height(), lanes, 0.0f);
    sum.y.fill(iof.y.width(), iof.y.height(), lanes, 0.0f);
  }

  auto freq = float32v(data.freq);
  auto amp  = float32v(data.amp);

  // modify domain if we have a domain modifier
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);

  auto iof_x_data = iof.x.data();
  auto iof_y_data = iof.y.data();
  auto sum_x_data = sum.x.data();
  auto sum_y_data = sum.y.data();

  for (uint32_t ii = 0; ii < iof.x.size(); ii += lanes)
  {
    auto x = hn::Load(vtag, iof_x_data + ii);
    auto y = hn::Load(vtag, iof_y_data + ii);

    auto sx = hn::Load(vtag, sum_x_data + ii);
    auto sy = hn::Load(vtag, sum_y_data + ii);

    sx = x * amp + sx;
    sy = y * amp + sy;

    hn::Store(sx, vtag, iof_x_data + ii);
    hn::Store(sy, vtag, iof_y_data + ii);
  }

  data.inputs[threadGroupId] = iof;
}

void domainWarpBounded(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  modifyDomain(node, pipe, threadGroupId);
  auto& inp        = pipe.getInput(threadGroupId, lanes, true);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();
  auto  wamp       = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value);
  auto  wfreq      = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value);
  auto  octaves    = std::get<ScalarValue>(node.param(3)).ivalue;
  auto  lacunarity = hn::Set(vtag, std::get<ScalarValue>(node.param(4)).value);
  auto  gain       = hn::Set(vtag, std::get<ScalarValue>(node.param(5)).value);
  auto  wstrength  = hn::Set(vtag, std::get<ScalarValue>(node.param(6)).value);
  auto  bounding   = hn::Set(vtag, std::get<ScalarValue>(node.param(7)).value);
  auto  seed       = hn::Set(itag, pipe.seed(threadGroupId));
  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto amp     = bounding * wamp;
    auto freq    = wfreq;
    auto seedInc = seed;

    auto sx       = x;
    auto sy       = y;
    auto strength = domainWarpInput<true>(seedInc, amp, x * freq, y * freq, sx, sy);
    for (int oct = 1; oct < octaves; ++oct)
    {
      seedInc -= int32v(-1);
      freq *= lacunarity;
      amp *= lerp(float32v(1), float32v(1) - strength, wstrength);
      amp *= gain;
      strength = domainWarpInput<true>(seedInc, amp, x * freq, y * freq, sx, sy);
    }

    hn::Store(sx, vtag, inp_x_data + ii);
    hn::Store(sy, vtag, inp_y_data + ii);
  }

  finish(node, pipe, threadGroupId);
}

} // namespace terra::HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(domainRotate);
HWY_EXPORT(domainScaleOffset);
HWY_EXPORT(domainWarp);
HWY_EXPORT(domainWarpBounded);

void domainWarp_prepare(Node& node, Pipeline_hwy& pipe)
{
  auto& mf = pipe.addCacheData<DomainFractal>(node.getSelf());
  mf.amp   = 1.f;
  mf.freq  = pipe.origFrequency();
  for (uint32_t i = 0; i < pipe.getNumThreads(); ++i)
    mf.inputs.emplace_at(i, hwyvb());
}

void domainWarp_end(Node& node, Pipeline_hwy& pipe)
{
  auto& mf         = pipe.getCacheData<DomainFractal>(node.getSelf());
  auto  octaves    = std::get<ScalarValue>(node.param(1)).ivalue - 1;
  auto  lacunarity = std::get<ScalarValue>(node.param(2)).value;
  auto  gain       = std::get<ScalarValue>(node.param(3)).value;
  mf.freq *= lacunarity;
  mf.amp *= gain;
  if (pipe.getIteration() < octaves)
    pipe.reissue();
}

void Domain_hwy()
{
  constexpr auto min = -std::numeric_limits<float>::infinity();
  constexpr auto max = std::numeric_limits<float>::infinity();

  // Common
  NodeMeta_hwy meta;
  meta.category    = "@Domain"_ls;
  meta.style       = "domain";
  meta.format.type = DataType::eInput;

  // domainRotate
  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(domainRotate);
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, -180.0f, 180.f), "@angle");
  get().addMeta("@domainRotate", meta);
  meta.parameterDef.resize(1);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(domainScaleOffset);
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(1.0f, -10000.f, 10000.f), "@scale");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(0.0f, min, max), "@offset");
  get().addMeta("@domainScaleOffset", meta);
  meta.parameterDef.resize(1);

  meta.icon    = "\xef\x87\xbe";
  meta.fn      = HWY_DYNAMIC_DISPATCH(domainWarp);
  meta.prepare = &domainWarp_prepare;
  meta.endIt   = &domainWarp_end;
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1, 1, 32), "@octaves");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(2.0f, min, max), "@lacunarity");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.5f, 0.f, .99f), "@gain");
  get().addMeta("@domainWarp", meta);

  meta.fn      = HWY_DYNAMIC_DISPATCH(domainWarpBounded);
  meta.prepare = nullptr;
  meta.endIt   = nullptr;
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@strength");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@bounds");
  get().addMeta("@domainWarpBounded", meta);
}

} // namespace terra

#endif
