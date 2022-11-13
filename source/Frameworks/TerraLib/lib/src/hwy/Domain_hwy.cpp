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

  auto& dst_inp = pipe.getInput(threadGroupId, lanes, true);
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);
  auto dst_x = dst_inp.x.data();
  auto dst_y = dst_inp.y.data();

  auto rad = radians(std::get<ScalarValue>(node.param(1)).value);
  auto cos = hn::Set(vtag, std::cos(rad));
  auto sin = hn::Set(vtag, std::sin(rad));

  for (uint32_t ii = 0; ii < dst_inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, dst_x + ii);
    const auto y = hn::Load(vtag, dst_y + ii);

    auto nx = hn::MulSub(cos, x, sin * y);
    auto ny = hn::MulAdd(sin, x, cos * y);

    hn::Store(nx, vtag, dst_x + ii);
    hn::Store(ny, vtag, dst_y + ii);
  }
}

void domainScaleOffset(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& dst_inp = pipe.getInput(threadGroupId, lanes, true);
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);
  auto dst_x = dst_inp.x.data();
  auto dst_y = dst_inp.y.data();

  auto sx = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[0]);
  auto sy = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[1]);
  auto ox = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value2[0]);
  auto oy = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value2[1]);

  for (uint32_t ii = 0; ii < dst_inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, dst_x + ii);
    const auto y = hn::Load(vtag, dst_y + ii);

    auto nx = hn::MulAdd(sx, x, ox);
    auto ny = hn::MulAdd(sy, y, oy);

    hn::Store(nx, vtag, dst_x + ii);
    hn::Store(ny, vtag, dst_y + ii);
  }
}

void domainWarp(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& data     = pipe.getCacheData<DomainFractal>(node.getSelf());
  auto  origSeed = pipe.swapSeed(data.seed, threadGroupId);
  auto  seed     = hn::Set(itag, data.seed);
  auto& iof      = pipe.getInput(threadGroupId, lanes, true);

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

    domainWarpInput<true>(seed, amp, x * freq, y * freq, sx, sy);

    hn::Store(sx, vtag, iof_x_data + ii);
    hn::Store(sy, vtag, iof_y_data + ii);
  }

  data.inputs[threadGroupId] = iof;
  pipe.swapSeed(origSeed, threadGroupId);
}

void domainWarpBounded(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto& data       = pipe.getCacheData<DomainFractal>(node.getSelf());
  auto  origSeed   = pipe.swapSeed(data.seed, threadGroupId);
  auto  seed       = hn::Set(itag, data.seed);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();
  auto  strength   = hn::Set(vtag, std::get<ScalarValue>(node.param(6)).value);
  auto  bounding   = std::get<ScalarValue>(node.param(7)).value;

  auto& sum        = data.inputs[threadGroupId];
  auto  sum_x_data = sum.x.data();
  auto  sum_y_data = sum.y.data();

  if (pipe.getIteration() == 0)
  {
    sum.x.fill(inp.x.width(), inp.x.height(), lanes, 0.0f);
    sum.y.fill(inp.y.width(), inp.y.height(), lanes, 0.0f);
  }

  auto freq = float32v(data.freq);
  auto amp  = float32v(data.amp);

  // modify domain if we have a domain modifier
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);

  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto sx = hn::Load(vtag, sum_x_data + ii);
    auto sy = hn::Load(vtag, sum_y_data + ii);

    domainWarpInput<false>(seed, amp, x * freq, y * freq, sx, sy);

    hn::Store(sx, vtag, inp_x_data + ii);
    hn::Store(sy, vtag, inp_y_data + ii);
  }

  data.inputs[threadGroupId] = inp;
  pipe.swapSeed(origSeed, threadGroupId);
}

void outputToDomain(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);
  inp.x     = out_a;
  inp.y     = out_b;
  pipe.popOutput(threadGroupId);
}

void outputToDomainScaled(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);
  auto& inp        = pipe.getInput(threadGroupId, lanes, true);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);
  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto a = hn::Load(vtag, out_a_data + ii);
    const auto b = hn::Load(vtag, out_b_data + ii);
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);
    hn::Store(x * a, vtag, inp_x_data + ii);
    hn::Store(y * b, vtag, inp_y_data + ii);
  }
  pipe.popOutput(threadGroupId);
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
HWY_EXPORT(outputToDomain);
HWY_EXPORT(outputToDomainScaled);

void domainWarp_prepare(Node& node, Pipeline_hwy& pipe)
{
  auto& mf = pipe.addCacheData<DomainFractal>(node.getSelf());
  mf.amp   = 1.f;
  mf.freq  = pipe.origFrequency();
  mf.seed  = pipe.origSeed();
  for (uint32_t i = 0; i < pipe.getNumThreads(); ++i)
    mf.inputs.emplace_at(i, hwyvb());
}

void domainWarp_end(Node& node, Pipeline_hwy& pipe)
{
  auto& mf         = pipe.getCacheData<DomainFractal>(node.getSelf());
  auto  octaves    = std::get<ScalarValue>(node.param(1)).ivalue - 1;
  auto  lacunarity = std::get<ScalarValue>(node.param(2)).value;
  auto  gain       = std::get<ScalarValue>(node.param(3)).value;
  auto  seed       = std::get<ScalarValue>(node.param(4)).ivalue;
  mf.freq *= lacunarity;
  mf.amp *= gain;
  mf.seed += seed;
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
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1), "@seedOffset");
  get().addMeta("@domainWarp", meta);

  meta.fn = HWY_DYNAMIC_DISPATCH(domainWarpBounded);
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@strength");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@bounds");
  get().addMeta("@domainWarpBounded", meta);
  meta.parameterDef.resize(1);
  meta.prepare = nullptr;
  meta.endIt   = nullptr;

  meta.fn = HWY_DYNAMIC_DISPATCH(outputToDomainScaled);
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(0.0f, min, max), "@source");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(0.0f, min, max), "@source2");
  get().addMeta("@outputToDomainScaled", meta);

  //  Remove 0
  meta.parameterDef.resize(0);
  meta.fn = HWY_DYNAMIC_DISPATCH(outputToDomain);
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(0.0f, min, max), "@source");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(0.0f, min, max), "@source2");
  get().addMeta("@outputToDomain", meta);
}

} // namespace terra

#endif
