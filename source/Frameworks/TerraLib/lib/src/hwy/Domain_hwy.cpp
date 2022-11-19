#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Domain_hwy.cpp"

#include "Common.h"
#include "Icons.h"
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
    pipe.reissue(node.getSelf());
}

void Domain_hwy()
{
  auto builder = buildMeta<NodeMeta_hwy>("@Domain"_ls, "domain");
  builder.outputs(DataFormat(DataType::eInput));

  {
    builder.add<DomainRotateNode>("@domainRotate", IconDomainRotate);
    builder.fn(HWY_DYNAMIC_DISPATCH(domainRotate));
    builder.param<&DomainRotateNode::angle>("@angle");
    builder.done();
  }

  {
    builder.add<DomainScaleOffsetNode>("@domainScaleOffset", IconDomainScaleOffset);
    builder.fn(HWY_DYNAMIC_DISPATCH(domainScaleOffset));
    builder.param<&DomainScaleOffsetNode::scale>("@scale");
    builder.param<&DomainScaleOffsetNode::offset>("@offset");
    builder.done();
  }

  {
    builder.add<DomainWarpNode>("@domainWarp", IconDomainWarp);
    builder.fn(HWY_DYNAMIC_DISPATCH(domainWarp));
    builder.param<&DomainWarpNode::octaves>("@octaves");
    builder.param<&DomainWarpNode::lacunarity>("@lacunarity");
    builder.param<&DomainWarpNode::gain>("@gain");
    builder.param<&DomainWarpNode::seedOffset>("@seedOffset");
    builder.prepare(domainWarp_prepare);
    builder.end(domainWarp_end);
    builder.done();
  }
  {
    builder.add<DomainWarpBoundedNode>("@domainWarpBounded", IconDomainWarpBounded);
    builder.fn(HWY_DYNAMIC_DISPATCH(domainWarpBounded));
    builder.param<&DomainWarpBoundedNode::octaves>("@octaves");
    builder.param<&DomainWarpBoundedNode::lacunarity>("@lacunarity");
    builder.param<&DomainWarpBoundedNode::gain>("@gain");
    builder.param<&DomainWarpBoundedNode::seedOffset>("@seedOffset");
    builder.param<&DomainWarpBoundedNode::gain>("@strength");
    builder.param<&DomainWarpBoundedNode::seedOffset>("@bounds");
    builder.prepare(domainWarp_prepare);
    builder.end(domainWarp_end);
    builder.done();
  }

  {
    builder.add<DomainWarpBoundedNode>("@outputToDomain", IconDomainOutput);
    builder.fn(HWY_DYNAMIC_DISPATCH(outputToDomain));
    builder.param<&OutputToDomain::source>("@source");
    builder.param<&OutputToDomain::source2>("@source2");
    builder.done();
  }
}

} // namespace terra

#endif
