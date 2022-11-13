
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Noise_hwy.cpp"

#include "Common.h"
#include "Node.h"
#include <hwy/foreach_target.h>

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Noise_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/Utility_hwy.h"

#include "Terra.h"

HWY_BEFORE_NAMESPACE();
namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

void simplex(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto      lanes = (uint32)hn::Lanes(vtag);
  constexpr float SQRT3 = 1.7320508075688772935274463415059f;

  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed(threadGroupId));
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
    auto n1 = gradientDot(hashPrimes(seed, FS_MaskedAdd_i32(i, hn::Set(itag, consts::X), i1),
                                     FS_NMaskedAdd_i32(j, hn::Set(itag, consts::Y), i1)),
                          x1, y1);
    auto n2 =
      gradientDot(hashPrimes(seed, hn::Add(i, hn::Set(itag, consts::X)), hn::Add(j, hn::Set(itag, consts::Y))), x2, y2);

    hn::Store(hn::Mul(hn::Set(vtag, 38.283687591552734375f), FS_FMulAdd_f32(n0, t0, FS_FMulAdd_f32(n1, t1, n2 * t2))),
              vtag, out_data + ii);
  }

  finish(node, pipe, threadGroupId);
}

// 2D simplex noise
void noise(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed(threadGroupId));

  auto const& perm = pipe.getConstants().perm;

  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    // Skew the input space to determine which simplex cell we're in
    auto s  = (x + y) * hn::Set(vtag, F2); // Hairy factor for 2D
    auto xs = x + s;
    auto ys = y + s;
    auto i  = hn::Floor(xs);
    auto j  = hn::Floor(ys);

    auto t  = (i + j) * hn::Set(vtag, G2);
    auto X0 = i - t; // Unskew the cell origin back to (x,y) space
    auto Y0 = j - t;
    auto x0 = x - X0; // The x,y distances from the cell origin
    auto y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    auto mask = (x0 > y0);

    auto i1 = hn::IfThenElse(x0 > y0, hn::Set(vtag, 1.0f), hn::Zero(vtag));
    auto j1 = hn::IfThenElse(x0 > y0, hn::Zero(vtag), hn::Set(vtag, 1.0f));

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6

    auto x1 = x0 - i1 + hn::Set(vtag, G2); // Offsets for middle corner in (x,y) unskewed coords
    auto y1 = y0 - j1 + hn::Set(vtag, G2);
    auto x2 = x0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2); // Offsets for last corner in (x,y) unskewed coords
    auto y2 = y0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2);
    auto ti = hn::ConvertTo(itag, i);
    auto tj = hn::ConvertTo(itag, j);

    // Wrap the integer indices at 256, to avoid indexing details::perm[] out of bounds

    // Calculate the contribution from the three corners

    auto t0   = hn::Set(vtag, 0.5f) - x0 * x0 - y0 * y0;
    auto t0t0 = t0 * t0;
    auto n0   = hn::IfNegativeThenElse(
      t0, hn::Zero(vtag),
      t0t0 * t0t0 *
        grad(hn::GatherIndex(
               itag, perm.data(),
               hn::And(ti + hn::GatherIndex(itag, perm.data(), hn::And(tj, hn::Set(itag, 0xff))), hn::Set(itag, 0xff))),
               x0, y0));

    auto t1   = hn::Set(vtag, 0.5f) - x1 * x1 - y1 * y1;
    auto t1t1 = t1 * t1;
    auto n1   = hn::IfNegativeThenElse(
      t1, hn::Zero(vtag),
      t1t1 * t1t1 *
        grad(hn::GatherIndex(
               itag, perm.data(),
               hn::And(ti + hn::ConvertTo(itag, i1) +
                           hn::GatherIndex(itag, perm.data(), hn::And(tj + hn::ConvertTo(itag, j1), hn::Set(itag, 0xff))),
                         hn::Set(itag, 0xff))),
               x1, y1));

    auto t2   = hn::Set(vtag, 0.5f) - x2 * x2 - y2 * y2;
    auto t2t2 = t2 * t2;
    auto n2   = hn::IfNegativeThenElse(
      t2, hn::Zero(vtag),
      t2t2 * t2t2 *
        grad(hn::GatherIndex(
               itag, perm.data(),
               hn::And(ti + hn::Set(itag, 1) +
                           hn::GatherIndex(itag, perm.data(), hn::And(tj + hn::Set(itag, 1), hn::Set(itag, 0xff))),
                         hn::Set(itag, 0xff))),
               x2, y2));

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    hn::Store(hn::Mul(hn::Set(vtag, 38.283687591552734375f), n0 + n1 + n2), vtag, out_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void openSimplex2(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto  lanes = (uint32)hn::Lanes(vtag);
  const float SQRT3 = 1.7320508075f;
  const float F2    = 0.5f * (SQRT3 - 1.0f);
  const float G2    = (3.0f - SQRT3) / 6.0f;
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed(threadGroupId));
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
    x0     = hn::Sub(x, hn::Sub(x0, g));
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
    auto n1 = gradientDotFancy(
      hashPrimes(seed, FS_MaskedAdd_i32(i, int32v(consts::X), i1), FS_NMaskedAdd_i32(j, int32v(consts::Y), i1)), x1,
      y1);
    auto n2 = gradientDotFancy(hashPrimes(seed, i + int32v(consts::X), j + int32v(consts::Y)), x2, y2);

    hn::Store(float32v(49.918426513671875f) * FS_FMulAdd_f32(n0, t0, FS_FMulAdd_f32(n1, t1, n2 * t2)), vtag,
              out_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void ridgedNoise(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_a_data = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed(threadGroupId));

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(vtag, out_a_data + i);
    const auto b = hn::Load(vtag, out_b_data + i);
    const auto c = a - hn::Abs(b);
    auto const r = hn::Mul(c, c);
    hn::Store(r, vtag, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void worlyNoise(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out        = pipe.getOutput(threadGroupId, lanes);
  auto& inp        = pipe.getInput(threadGroupId, lanes, true);
  auto& out_a      = pipe.pushOutput(threadGroupId, lanes);
  auto  out_a_data = out_a.data();
  auto  out_data   = out.data();
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();
  auto  falloff    = -std::get<ScalarValue>(node.param(2)).value;

  auto& iof        = pipe.pushInput(threadGroupId, lanes, false);
  auto  iof_x_data = iof.x.data();
  auto  iof_y_data = iof.y.data();
  float freq       = pipe.getThreadData(threadGroupId).params.frequency;

  out.fill(0.0f);
  for (int j = -1; j <= 1; j++)
  {
    for (int i = -1; i <= 1; i++)
    {
      for (uint32_t ii = 0; ii < out.size(); ii += lanes)
      {
        auto const x  = hn::Load(vtag, inp_x_data + ii);
        auto const y  = hn::Load(vtag, inp_y_data + ii);
        auto const px = hn::Floor(x);
        auto const py = hn::Floor(y);

        hn::Store(px * float32v(freq * i), vtag, iof_x_data + ii);
        hn::Store(py * float32v(freq * j), vtag, iof_y_data + ii);
      }

      NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

      for (uint32_t ii = 0; ii < out.size(); ii += lanes)
      {
        auto const x   = hn::Load(vtag, inp_x_data + ii);
        auto const y   = hn::Load(vtag, inp_y_data + ii);
        auto const px  = hn::Floor(x);
        auto const py  = hn::Floor(y);
        auto const fx  = x - px;
        auto const fy  = y - py;
        auto       bx  = float32v(freq * i);
        auto       by  = float32v(freq * j);
        auto       n   = hn::MulAdd(hn::Load(vtag, out_a_data + ii), float32v(0.5f), float32v(0.5f));
        auto const rx  = bx - fx + n;
        auto const ry  = by - fy + n;
        auto       len = hn::Sqrt(rx * rx + ry * ry);

        auto sum = hn::Load(vtag, out_data + ii);
        sum += hn::Exp(vtag, len * float32v(falloff));
        hn::Store(sum, vtag, out_data + ii);
      }
    }
  }

  falloff = 1.f / falloff;

  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    auto res = hn::Load(vtag, out_data + ii);
    hn::Store(hn::Log(vtag, res) * float32v(falloff), vtag, out_data + ii);
  }

  pipe.popInput(threadGroupId);
  pipe.popOutput(threadGroupId);
  finish(node, pipe, threadGroupId);
}

void flowNoise(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto seed       = hn::Set(itag, pipe.seed(threadGroupId));

  auto const& perm  = pipe.getConstants().perm;
  auto        angle = radians(std::get<ScalarValue>(node.param(1)).value);
  auto        sin   = hn::Sin(vtag, float32v(angle));
  auto        cos   = hn::Cos(vtag, float32v(angle));
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    // Skew the input space to determine which simplex cell we're in
    auto s  = (x + y) * hn::Set(vtag, F2); // Hairy factor for 2D
    auto xs = x + s;
    auto ys = y + s;
    auto i  = hn::Floor(xs);
    auto j  = hn::Floor(ys);

    auto t  = (i + j) * hn::Set(vtag, G2);
    auto X0 = i - t; // Unskew the cell origin back to (x,y) space
    auto Y0 = j - t;
    auto x0 = x - X0; // The x,y distances from the cell origin
    auto y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    auto mask = (x0 > y0);

    auto i1 = hn::IfThenElse(x0 > y0, hn::Set(vtag, 1.0f), hn::Zero(vtag));
    auto j1 = hn::IfThenElse(x0 > y0, hn::Zero(vtag), hn::Set(vtag, 1.0f));

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6

    auto x1 = x0 - i1 + hn::Set(vtag, G2); // Offsets for middle corner in (x,y) unskewed coords
    auto y1 = y0 - j1 + hn::Set(vtag, G2);
    auto x2 = x0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2); // Offsets for last corner in (x,y) unskewed coords
    auto y2 = y0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2);
    auto ti = hn::ConvertTo(itag, i);
    auto tj = hn::ConvertTo(itag, j);

    // Wrap the integer indices at 256, to avoid indexing details::perm[] out of bounds

    // Calculate the contribution from the three corners

    /// ------
    auto t0  = hn::Set(vtag, 0.5f) - x0 * x0 - y0 * y0;
    auto gi  = hn::And(hn::GatherIndex(itag, perm.data(),
                                       hn::And(ti + hn::GatherIndex(itag, perm.data(), hn::And(tj, hn::Set(itag, 0xff))),
                                               hn::Set(itag, 0xff))),
                       hn::Set(itag, 7));
    auto t20 = t0 * t0;
    auto t40 = t20 * t20;
    auto n0  = hn::IfNegativeThenElse(t0, hn::Zero(vtag), t40 * graddotp2(gradrotXY(gi, sin, cos), x0, y0));

    /// ------
    auto t1  = hn::Set(vtag, 0.5f) - x1 * x1 - y1 * y1;
    gi       = hn::And(hn::GatherIndex(itag, perm.data(),
                                       hn::And(ti + hn::ConvertTo(itag, i1) +
                                                 hn::GatherIndex(itag, perm.data(),
                                                                 hn::And(tj + hn::ConvertTo(itag, j1), hn::Set(itag, 0xff))),
                                               hn::Set(itag, 0xff))),
                       hn::Set(itag, 7));
    auto t21 = t1 * t1;
    auto t41 = t21 * t21;
    auto n1  = hn::IfNegativeThenElse(t1, hn::Zero(vtag), t41 * graddotp2(gradrotXY(gi, sin, cos), x1, y1));

    /// ------

    auto t2 = hn::Set(vtag, 0.5f) - x2 * x2 - y2 * y2;
    gi      = hn::And(
      hn::GatherIndex(itag, perm.data(),
                           hn::And(ti + hn::Set(itag, 1) +
                                     hn::GatherIndex(itag, perm.data(), hn::And(tj + hn::Set(itag, 1), hn::Set(itag, 0xff))),
                                   hn::Set(itag, 0xff))),
      hn::Set(itag, 7));
    auto t22 = t2 * t2;
    auto t42 = t22 * t22;
    auto n2  = hn::IfNegativeThenElse(t2, hn::Zero(vtag), t42 * graddotp2(gradrotXY(gi, sin, cos), x2, y2));

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    hn::Store(hn::Mul(hn::Set(vtag, 38.283687591552734375f), n0 + n1 + n2), vtag, out_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void multiFractal(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes    = (uint32)hn::Lanes(vtag);
  auto&      data     = pipe.getCacheData<MultiFractal>(node.getSelf());
  auto       origFreq = pipe.swapFreq(data.freq, threadGroupId);
  auto       origSeed = pipe.swapSeed(data.seed, threadGroupId);
  auto&      out      = pipe.getOutput(threadGroupId, lanes);
  auto&      iof      = pipe.pushInput(threadGroupId, lanes, true);
  auto       out_data = out.data();
  auto       seed     = hn::Set(itag, pipe.seed(threadGroupId));

  auto& save      = data.outputs[threadGroupId];
  auto& mod       = pipe.pushOutput(threadGroupId, lanes);
  auto  iteration = pipe.getIteration();

  if (pipe.getIteration() == 0)
  {
    save.fill(out.width(), out.height(), lanes, 0.0f);
  }

  auto freq = float32v(data.freq);
  auto amp  = float32v(data.amp);

  // modify domain if we have a domain modifier
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto save_data = save.data();
  auto mod_data  = mod.data();

  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    auto n   = hn::Load(vtag, mod_data + ii);
    auto sum = hn::Load(vtag, save_data + ii);
    n        = n * amp + sum;
    hn::Store(n, vtag, out_data + ii);
  }

  data.outputs[threadGroupId] = out;

  pipe.popOutput(threadGroupId);
  pipe.popInput(threadGroupId);
  pipe.swapFreq(origFreq, threadGroupId);
  pipe.swapSeed(origSeed, threadGroupId);
}

void dnoiseFractal(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto  lanes    = (uint32)hn::Lanes(vtag);
  auto&       out      = pipe.getOutput(threadGroupId, lanes);
  auto&       data     = pipe.getCacheData<DerivFractal>(node.getSelf());
  auto        origFreq = pipe.swapFreq(data.freq, threadGroupId);
  auto        origSeed = pipe.swapSeed(data.seed, threadGroupId);
  auto&       iof      = pipe.pushInput(threadGroupId, lanes, false);
  auto        out_data = out.data();
  auto        seed     = hn::Set(itag, pipe.seed(threadGroupId));
  auto const& perm     = pipe.getConstants().perm;
  auto&       savedx   = data.dx[threadGroupId];
  auto&       sum      = data.sum[threadGroupId];
  float       amp      = data.amp;
  float       freq     = data.freq;

  if (pipe.getIteration() == 0)
  {
    savedx.fill(out.width(), out.height(), lanes, 0.0f);
    sum.fill(out.width(), out.height(), lanes, 0.0f);
  }

  // modify domain if we have a domain modifier
  NodeMeta_hwy::domain(node.param(0), pipe, threadGroupId, lanes);
  auto dxdata     = savedx.data();
  auto sum_data   = sum.data();
  auto iof_x_data = iof.x.data();
  auto iof_y_data = iof.y.data();
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x  = hn::Load(vtag, iof_x_data + ii);
    const auto y  = hn::Load(vtag, iof_y_data + ii);
    auto       d  = dnoise(x, y, perm);
    auto       dx = hn::Load(vtag, dxdata + ii);
    dx += d.dx;
    auto sum = hn::Load(vtag, sum_data + ii);
    sum      = sum + hn::Div(float32v(amp) * d.h, hn::MulAdd(float32v(2.f), dx * dx, float32v(1.f)));
    hn::Store(sum, vtag, out_data + ii);
    hn::Store(dx, vtag, dxdata + ii);
  }

  data.sum[threadGroupId] = out;

  pipe.popInput(threadGroupId);
  pipe.swapFreq(origFreq, threadGroupId);
  pipe.swapSeed(origSeed, threadGroupId);
}

void cellularValue(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out        = pipe.getOutput(threadGroupId, lanes);
  auto  out_data   = out.data();
  auto& inp        = pipe.getInput(threadGroupId, lanes, true);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();
  auto& jitter     = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  int  valIdx      = std::get<ScalarValue>(node.param(2)).ivalue;
  auto jitter_data = jitter.data();
  auto seed        = hn::Set(itag, pipe.getThreadData(threadGroupId).params.seed);
  auto dist        = [&]<int DistFunc>(std::integral_constant<int, DistFunc>)
  {
    for (uint32_t ii = 0; ii < out.size(); ii += lanes)
    {
      const auto x = hn::Load(vtag, inp_x_data + ii);
      const auto y = hn::Load(vtag, inp_y_data + ii);
      auto       j = hn::Load(vtag, jitter_data + ii);
      hn::Store(cellularValueNoise<DistFunc>(seed, j, x, y, valIdx), vtag, out_data + ii);
    }
  };
  switch (std::get<ScalarValue>(node.param(3)).ivalue)
  {
  case 0:
    dist(std::integral_constant<int, 0>());
    break;
  case 1:
    dist(std::integral_constant<int, 1>());
    break;
  case 2:
    dist(std::integral_constant<int, 2>());
    break;
  case 3:
    dist(std::integral_constant<int, 3>());
    break;
  default:
  case 4:
    dist(std::integral_constant<int, 4>());
    break;
  }
  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

} // namespace terra::HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(simplex);
HWY_EXPORT(openSimplex2);
HWY_EXPORT(noise);
HWY_EXPORT(ridgedNoise);
HWY_EXPORT(worlyNoise);
HWY_EXPORT(flowNoise);
HWY_EXPORT(multiFractal);
HWY_EXPORT(dnoiseFractal);
HWY_EXPORT(cellularValue);

void multiFractal_prepare(Node& node, Pipeline_hwy& pipe)
{
  auto& mf = pipe.addCacheData<MultiFractal>(node.getSelf());
  mf.amp   = 0.5f;
  mf.freq  = pipe.origFrequency();
  mf.seed  = pipe.origSeed();
  for (uint32_t i = 0; i < pipe.getNumThreads(); ++i)
    mf.outputs.emplace_at(i, hwybuffer());
}

void multiFractal_end(Node& node, Pipeline_hwy& pipe)
{
  auto& mf         = pipe.getCacheData<MultiFractal>(node.getSelf());
  auto  octaves    = std::get<ScalarValue>(node.param(2)).ivalue - 1;
  auto  lacunarity = std::get<ScalarValue>(node.param(3)).value;
  auto  gain       = std::get<ScalarValue>(node.param(4)).value;
  auto  seed       = std::get<ScalarValue>(node.param(5)).ivalue;
  mf.freq *= lacunarity;
  mf.amp *= gain;
  mf.seed += seed;
  if (pipe.getIteration() < octaves)
    pipe.reissue();
}

void dnoiseFractal_prepare(Node& node, Pipeline_hwy& pipe)
{
  auto& mf = pipe.addCacheData<DerivFractal>(node.getSelf());
  mf.amp   = 0.5f;
  mf.freq  = pipe.origFrequency();
  mf.seed  = pipe.origSeed();
  for (uint32_t i = 0; i < pipe.getNumThreads(); ++i)
  {
    mf.sum.emplace_at(i, hwybuffer());
    mf.dx.emplace_at(i, hwybuffer());
  }
}

void dnoiseFractal_end(Node& node, Pipeline_hwy& pipe)
{
  auto& mf         = pipe.getCacheData<DerivFractal>(node.getSelf());
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

void Noise_hwy()
{
  constexpr auto min = -std::numeric_limits<float>::infinity();
  constexpr auto max = std::numeric_limits<float>::infinity();

  // Common
  NodeMeta_hwy meta;
  meta.category = "@Noise"_ls;
  meta.style    = "noise";

  // openSimplex
  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(openSimplex2);
  get().addMeta("@openSimplex", meta);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(simplex);
  get().addMeta("@simplex", meta);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(noise);
  get().addMeta("@noise", meta);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(ridgedNoise);
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@ridgeOffset");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@source");
  get().addMeta("@ridgedNoise", meta);
  meta.parameterDef.resize(1);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(worlyNoise);
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@source");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@falloff");
  get().addMeta("@worlyNoise", meta);
  meta.parameterDef.resize(1);

  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(flowNoise);
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.0f, -180.0f, 180.f), "@angle");
  get().addMeta("@flowNoise", meta);
  meta.parameterDef.resize(1);

  meta.icon    = "\xef\x87\xbe";
  meta.fn      = HWY_DYNAMIC_DISPATCH(multiFractal);
  meta.prepare = &multiFractal_prepare;
  meta.endIt   = &multiFractal_end;
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@source");
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1, 1, 256), "@octaves");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(2.0f, min, max), "@lacunarity");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.5f, 0.f, 0.99f), "@gain");
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1), "@seedOffset");
  get().addMeta("@multiFractal", meta);
  meta.parameterDef.resize(1);

  meta.icon    = "\xef\x87\xbe";
  meta.fn      = HWY_DYNAMIC_DISPATCH(dnoiseFractal);
  meta.prepare = &dnoiseFractal_prepare;
  meta.endIt   = &dnoiseFractal_end;
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1, 1, 256), "@octaves");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(2.0f, min, max), "@lacunarity");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(0.5f, 0.f, .99f), "@gain");
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(1), "@seedOffset");
  get().addMeta("@dnoiseFractal", meta);
  meta.parameterDef.resize(1);

  meta.icon    = "\xef\x87\xbe";
  meta.fn      = HWY_DYNAMIC_DISPATCH(cellularValue);
  meta.prepare = nullptr;
  meta.endIt   = nullptr;
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@jitter");
  meta.parameterDef.emplace_back(FmtVal<DataType::eInt>(0, 0, 3), "@returnType");
  meta.parameterDef.emplace_back(
    FmtEnum(0, {"@eucledian"_ls, "@eucledianSquared"_ls, "@manhattan"_ls, "@hybrid"_ls, "@maxAxis"_ls}),
    "@distanceType");
  get().addMeta("@cellularValue", meta);
  meta.parameterDef.resize(1);
}

} // namespace terra

#endif
