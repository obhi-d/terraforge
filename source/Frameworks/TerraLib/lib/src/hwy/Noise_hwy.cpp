
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Noise_hwy.cpp"

#include <hwy/foreach_target.h>

#include "NodeMeta.h"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Utility_hwy.hpp"
#include "hwy/Pipeline_hwy.h"

#include "Terra.h"
#include "metas/Noise.h"


HWY_BEFORE_NAMESPACE();
namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

void simplex(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId) 
{
  const V_t vtag{};
  const I_t itag{};

  const auto               lanes = (uint32)hn::Lanes(vtag);
  constexpr float SQRT3 = 1.7320508075688772935274463415059f;
  constexpr float F2    = 0.5f * (SQRT3 - 1.0f);
  constexpr float G2    = (3.0f - SQRT3) / 6.0f;

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed());
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);
    
    auto f  = hn::Mul(hn::Set(vtag, F2), hn::Add(x, y));
    auto x0 = hn::Floor(hn::Add(x, f));
    auto y0 = hn::Floor(hn::Add(y, f));
    
    auto i = hn::Mul(hn::ConvertTo(itag, x0), hn::Set(itag, consts::X));
    auto j = hn::Mul(hn::ConvertTo(itag, y0), hn::Set(itag, consts::Y));

    auto g = hn::Mul(hn::Set(vtag, G2), hn::Add(x0, y0));
    x0     = hn::Sub(x, hn::Sub(x0, g));
    y0     = hn::Sub(y, hn::Sub(y0, g));

    auto i1 = hn::Gt(x0, y0);
    // mask32v j1 = ~i1; //NMasked funcs

    auto x1 = hn::Add(FS_MaskedSub_f32(x0, hn::Set(vtag, 1.f), i1), hn::Set(vtag, G2));
    auto y1 = hn::Add(FS_NMaskedSub_f32(y0, hn::Set(vtag, 1.f), i1), hn::Set(vtag, G2));

    auto x2 = hn::Add(x0, hn::Set(vtag, G2 * 2 - 1));
    auto y2 = hn::Add(y0, hn::Set(vtag, G2 * 2 - 1));

    auto t0 = FS_FNMulAdd_f32(x0, x0, FS_FNMulAdd_f32(y0, y0, hn::Set(vtag, 0.5f)));
    auto t1 = FS_FNMulAdd_f32(x1, x1, FS_FNMulAdd_f32(y1, y1, hn::Set(vtag, 0.5f)));
    auto t2 = FS_FNMulAdd_f32(x2, x2, FS_FNMulAdd_f32(y2, y2, hn::Set(vtag, 0.5f)));

    t0 = FS_Max_f32(t0, hn::Zero(vtag));
    t1 = FS_Max_f32(t1, hn::Zero(vtag));
    t2 = FS_Max_f32(t2, hn::Zero(vtag));

    t0 = hn::Mul(t0, t0);
    t0 = hn::Mul(t0, t0);
    t1 = hn::Mul(t1, t1);
    t1 = hn::Mul(t1, t1);
    t2 = hn::Mul(t2, t2);
    t2 = hn::Mul(t2, t2);

    auto n0 = gradientDot(hashPrimes(seed, i, j), x0, y0);
    auto n1 = gradientDot(
      hashPrimes(seed, FS_MaskedAdd_i32(i, hn::Set(itag, consts::X), i1), FS_NMaskedAdd_i32(j, hn::Set(itag, consts::Y), i1)),
                                          x1, y1);
    auto n2 =
      gradientDot(hashPrimes(seed, hn::Add(i, hn::Set(itag, consts::X)), hn::Add(j, hn::Set(itag, consts::Y))), x2, y2);

    hn::Store(hn::Mul(hn::Set(vtag, 38.283687591552734375f) , FS_FMulAdd_f32(n0, t0, FS_FMulAdd_f32(n1, t1, n2 * t2))), vtag, out_data + ii);
  }
}

void openSimplex(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto  lanes = (uint32)hn::Lanes(vtag);
  const float SQRT3 = 1.7320508075f;
  const float F2    = 0.5f * (SQRT3 - 1.0f);
  const float G2    = (3.0f - SQRT3) / 6.0f;

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed());
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto f  = hn::Mul(hn::Set(vtag, F2), hn::Add(x, y));
    auto x0 = FS_Floor_f32(x + f);
    auto y0 = FS_Floor_f32(y + f);

    auto i = hn::Mul(FS_Convertf32_i32(x0), int32v(consts::X));
    auto j = hn::Mul(FS_Convertf32_i32(y0), int32v(consts::Y));

    auto g = hn::Mul(float32v(G2), hn::Add(x0, y0));
    x0         = hn::Sub(x, hn::Sub(x0, g));
    y0     = hn::Sub(y, hn::Sub(y0, g));

    auto i1 = hn::Gt(x0, y0);
    // mask32v j1 = ~i1; //NMasked funcs

    auto x1 = hn::Add(FS_MaskedSub_f32(x0, float32v(1.f), i1), float32v(G2));
    auto y1 = hn::Add(FS_NMaskedSub_f32(y0, float32v(1.f), i1), float32v(G2));
    auto x2 = hn::Add(x0, float32v((G2 * 2) - 1));
    auto y2 = hn::Add(y0, float32v((G2 * 2) - 1));

    auto t0 = float32v(0.5f) - (x0 * x0) - (y0 * y0);
    auto t1 = float32v(0.5f) - (x1 * x1) - (y1 * y1);
    auto t2 = float32v(0.5f) - (x2 * x2) - (y2 * y2);

    t0 = FS_Max_f32(t0, float32v(0));
    t1 = FS_Max_f32(t1, float32v(0));
    t2 = FS_Max_f32(t2, float32v(0));

    t0 = hn::Mul(t0, t0);
    t0 = hn::Mul(t0, t0);
    t1 = hn::Mul(t1, t1);
    t1 = hn::Mul(t1, t1);
    t2 = hn::Mul(t2, t2);
    t2 = hn::Mul(t2, t2);

    auto n0 = gradientDotFancy(hashPrimes(seed, i, j), x0, y0);
    auto n1 = gradientDotFancy(hashPrimes(seed, FS_MaskedAdd_i32(i, int32v(consts::X), i1), FS_NMaskedAdd_i32(j, int32v(consts::Y), i1)),
                                               x1, y1);
    auto n2 = gradientDotFancy(hashPrimes(seed, i + int32v(consts::X), j + int32v(consts::Y)), x2, y2);

    hn::Store(float32v(49.918426513671875f) * FS_FMulAdd_f32(n0, t0, FS_FMulAdd_f32(n1, t1, n2 * t2)),
              vtag, out_data + ii);
  }
}

} // namespace terra::HWY_NAMESPACE

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(simplex);
HWY_EXPORT(openSimplex);
void Noise_hwy() 
{
  // Common
  NodeMeta_hwy meta;
  meta.category = "@Noise"_ls;
  meta.style    = "Noise";

  // openSimplex
  meta.create = [](NodeMeta const& meta) -> std::shared_ptr<Node>
  {
    return std::make_shared<Noise>(meta);
  };

  meta.icon    = "\xef\x87\xbe";
  meta.name    = "@OpenSimplex"_ls;
  meta.tooltip = "@OpenSimplexTooltip"_ls;
  meta.help    = "@OpenSimplexHelp"_ls;
  meta.fn      = HWY_DYNAMIC_DISPATCH(openSimplex);
  get().addMeta("openSimplex", meta);

  meta.icon    = "\xef\x87\xbe";
  meta.name    = "@Simplex"_ls;
  meta.tooltip = "@SimplexTooltip"_ls;
  meta.help    = "@SimplexHelp"_ls;
  meta.fn      = HWY_DYNAMIC_DISPATCH(simplex);
  get().addMeta("simplex", meta);
}

} // namespace terra

#endif