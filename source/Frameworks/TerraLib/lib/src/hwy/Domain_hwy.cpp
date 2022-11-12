#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Domain_hwy.cpp"

#include "Common.h"
#include "Node.h"
#include <hwy/foreach_target.h>

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
  auto cos = hn::Set(vtag, std::cos(rad));
  auto sin = hn::Set(vtag, std::sin(rad));

  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto nx         = hn::MulSub(cos, x, sin * y);
    auto ny         = hn::MulAdd(sin, x, cos * y);

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
  auto sx        = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[0]);
  auto sy        = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value2[1]);
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

  modifyDomain(node, pipe, threadGroupId);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();
  auto  amp         = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value);
  auto  freq        = hn::Set(vtag, std::get<ScalarValue>(node.param(2)).value);
  auto  seed        = hn::Set(itag, pipe.seed());
  for (uint32_t ii = 0; ii < inp.x.size(); ii += lanes)
  {
    auto x = hn::Load(vtag, inp_x_data + ii);
    auto y = hn::Load(vtag, inp_y_data + ii);
          
    domainWarpInput<false>(seed, amp, x * freq, y * freq, x, y);

    hn::Store(x, vtag, inp_x_data + ii);
    hn::Store(y, vtag, inp_y_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void domainWarpFractal(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
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
  auto   seed      = hn::Set(itag, pipe.seed());
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
HWY_EXPORT(domainWarpFractal);

void Domain_hwy() 
{
  constexpr auto min = std::numeric_limits<float>::min();
  constexpr auto max = std::numeric_limits<float>::max();
  // Common
  NodeMeta_hwy meta;
  meta.category = "@Domain"_ls;
  meta.style    = "domain";
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
  meta.parameterDef.emplace_back(
    FmtVal<DataType::eFloat2>(0.0f, min, max), "@offset");
  get().addMeta("@domainScaleOffset", meta);
  meta.parameterDef.resize(1);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(domainWarp);
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@amplitude");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@frequency");
  get().addMeta("@domainWarp", meta);

  meta.fn = HWY_DYNAMIC_DISPATCH(domainWarpFractal);
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1, 1, 16), "@octaves");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@lacunarity");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@gain");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@strength");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, min, max), "@bounds");
  get().addMeta("@domainWarpFractal", meta);
}

}

#endif
