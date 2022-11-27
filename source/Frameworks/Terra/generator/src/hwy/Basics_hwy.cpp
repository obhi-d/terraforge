
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Basics_hwy.cpp"

#include "Common.h"
#include "CurveData.h"
#include "Icons.h"
#include "Image.h"
#include "Node.h"
#include <hwy/foreach_target.h>

#include "hwy/Basics_hwy.h"
#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/Utility_hwy.h"

#include "Terra.h"

HWY_BEFORE_NAMESPACE();
namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

void checkerBoard(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto multiplier = hn::Set(vtag, std::get<ScalarValue>(node.param(1)).value);
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto value = hn::Xor(FS_Convertf32_i32(x * multiplier), FS_Convertf32_i32(y * multiplier));

    hn::Store(hn::Xor(float32v(1.0f), FS_Casti32_f32(hn::ShiftLeft<31>(value))), vtag, out_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void curve(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto         out_data = out.data();
  const float* xy[2]    = {inp.x.data(), inp.y.data()};
  auto const&  param    = node.param(1);
  // if we dont have a curve, we just fill with the constant

  if (std::holds_alternative<Source>(param))
  {
    auto const& td     = pipe.getThreadData(threadGroupId);
    auto&       cd     = get().get<CurveData>(std::get<Source>(param).source);
    bool        allowX = std::get<ScalarValue>(node.param(2)).bvalue;
    bool        allowY = std::get<ScalarValue>(node.param(3)).bvalue;
    auto const& str    = std::get<ScalarValue>(node.param(4)).value2;

    auto doIt = [&]<bool applyX, bool applyY>(std::bool_constant<applyX>, std::bool_constant<applyY>)
    {
      auto xoffset   = hn::Set(vtag, td.uv.offset[0]);
      auto xrsize    = hn::Set(vtag, td.uv.recipSize[0]);
      auto xstrength = hn::Set(vtag, str[0]);
      auto yoffset   = hn::Set(vtag, td.uv.offset[1]);
      auto yrsize    = hn::Set(vtag, td.uv.recipSize[1]);
      auto ystrength = hn::Set(vtag, str[1]);
      for (uint32_t ii = 0; ii < out.size(); ii += lanes)
      {
        auto store = hn::Set(vtag, 0);
        if constexpr (applyX)
        {
          const auto   x      = hn::Load(vtag, xy[0] + ii);
          auto         u      = FS_SubMul(x, xoffset, xrsize);
          hn::Vec<V_t> storeV = hn::Set(vtag, 0);
          for (uint32_t l = 0; l < lanes; ++l)
            storeV = hn::InsertLane(storeV, l, cd.spline(hn::ExtractLane(u, l)));
          store = hn::Mul(storeV, xstrength);
        }
        if constexpr (applyY)
        {
          const auto   y      = hn::Load(vtag, xy[1] + ii);
          auto         u      = FS_SubMul(y, yoffset, yrsize);
          auto         storeX = hn::Set(vtag, 0);
          hn::Vec<V_t> storeV = hn::Set(vtag, 0);
          for (uint32_t l = 0; l < lanes; ++l)
            storeV = hn::InsertLane(storeV, l, cd.spline(hn::ExtractLane(u, l)));
          store += hn::Mul(storeV, ystrength);
        }

        hn::Store(store, vtag, out_data + ii);
      }
    };
    if (allowX && allowY)
      doIt(std::bool_constant<true>{}, std::bool_constant<true>{});
    else if (allowX)
      doIt(std::bool_constant<true>{}, std::bool_constant<false>{});
    else if (allowY)
      doIt(std::bool_constant<false>{}, std::bool_constant<true>{});
    else
      out.fill(0.f);
  }
  else
    out.fill(0.f);
  finish(node, pipe, threadGroupId);
}

void imageMask(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto         out_data = out.data();
  const float* xy[2]    = {inp.x.data(), inp.y.data()};
  auto const&  param    = node.param(1);
  // if we dont have a curve, we just fill with the constant

  if (std::holds_alternative<Source>(param))
  {
    auto const& td       = pipe.getThreadData(threadGroupId);
    auto&       cd       = get().get<Image>(std::get<Source>(param).source);
    int         sampling = std::get<ScalarValue>(node.param(2)).ivalue;
    auto        offset   = std::get<ScalarValue>(node.param(3)).value2;
    auto        scale    = std::get<ScalarValue>(node.param(4)).value2;
    auto        xoffset  = hn::Set(vtag, td.uv.offset[0]);
    auto        xrsize   = hn::Set(vtag, td.uv.recipSize[0]);
    auto        yoffset  = hn::Set(vtag, td.uv.offset[1]);
    auto        yrsize   = hn::Set(vtag, td.uv.recipSize[1]);
    auto        uoffset  = hn::Set(vtag, offset[0]);
    auto        uscale   = hn::Set(vtag, scale[0]);
    auto        voffset  = hn::Set(vtag, offset[1]);
    auto        vscale   = hn::Set(vtag, scale[1]);

    auto sfn = [&]<int sampling>(std::integral_constant<int, sampling>)
    {
      auto store = hn::Set(vtag, 0);
      for (uint32_t ii = 0; ii < out.size(); ii += lanes)
      {
        auto       store = hn::Set(vtag, 0);
        const auto x     = hn::Load(vtag, xy[0] + ii);
        auto       u     = FS_SubMul(x, xoffset, xrsize);
        const auto y     = hn::Load(vtag, xy[1] + ii);
        auto       v     = FS_SubMul(y, yoffset, yrsize);
        u                = hn::MulAdd(u, uscale, uoffset);
        v                = hn::MulAdd(v, vscale, voffset);
        for (uint32_t l = 0; l < lanes; ++l)
        {
          if constexpr (sampling == 1)
            store = hn::InsertLane(store, l, cd.sample(hn::ExtractLane(u, l), hn::ExtractLane(v, l)));
          else
            store = hn::InsertLane(store, l, cd.sampleN<sampling>(hn::ExtractLane(u, l), hn::ExtractLane(v, l)));
        }
        hn::Store(store, vtag, out_data + ii);
      }
    };
    switch (sampling)
    {
    case 1:
      sfn(std::integral_constant<int, 1>{});
      break;
    case 2:
      sfn(std::integral_constant<int, 2>{});
      break;
    case 3:
      sfn(std::integral_constant<int, 3>{});
      break;
    default:
    case 4:
      sfn(std::integral_constant<int, 4>{});
      break;
    }
  }
  else
    out.fill(0.f);
  finish(node, pipe, threadGroupId);
}

void sin(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data = out.data();

  auto const& amplitude  = std::get<ScalarValue>(node.param(1));
  auto const& phase      = std::get<ScalarValue>(node.param(2));
  auto        ampX       = hn::Set(vtag, amplitude.value2[0]);
  auto        ampY       = hn::Set(vtag, amplitude.value2[1]);
  auto        phaseX     = hn::Set(vtag, radians(phase.value2[0]));
  auto        phaseY     = hn::Set(vtag, radians(phase.value2[1]));
  auto        inp_x_data = inp.x.data();
  auto        inp_y_data = inp.y.data();
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);
    auto       store =
      hn::Add(hn::Mul(ampX, hn::Sin(vtag, hn::Add(x, phaseX))), hn::Mul(ampY, hn::Sin(vtag, hn::Add(y, phaseY))));
    hn::Store(store, vtag, out_data + ii);
  }
  finish(node, pipe, threadGroupId);
}

void distance(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto& out_x = pipe.getOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto& out_y = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);

  auto        out_x_data = out_x.data();
  auto        out_y_data = out_y.data();
  auto        inp_x_data = inp.x.data();
  auto        inp_y_data = inp.y.data();
  auto const& td         = pipe.getThreadData(threadGroupId);
  auto        freq       = hn::Set(vtag, td.params.frequency);
  auto dist = [&]<int DistFunc, bool Modulate>(std::integral_constant<int, DistFunc>, std::bool_constant<Modulate>)
  {
    for (uint32_t ii = 0; ii < out_x.size(); ii += lanes)
    {
      const auto x  = hn::Load(vtag, inp_x_data + ii);
      const auto y  = hn::Load(vtag, inp_y_data + ii);
      auto       sx = hn::Load(vtag, out_x_data + ii);
      auto       sy = hn::Load(vtag, out_y_data + ii);
      if constexpr (Modulate)
      {
        sx = hn::Mul(sx, freq);
        sy = hn::Mul(sy, freq);
      }
      auto store = Distance<DistFunc>(hn::Sub(x, sx), hn::Sub(y, sy));
      hn::Store(store, vtag, out_x_data + ii);
    }
  };

  auto disp = [&]<bool modulate>(std::bool_constant<modulate> m)
  {
    switch (std::get<ScalarValue>(node.param(4)).ivalue)
    {
    case 0:
      dist(std::integral_constant<int, 0>(), m);
      break;
    case 1:
      dist(std::integral_constant<int, 1>(), m);
      break;
    case 2:
      dist(std::integral_constant<int, 2>(), m);
      break;
    case 3:
      dist(std::integral_constant<int, 3>(), m);
      break;
    default:
    case 4:
      dist(std::integral_constant<int, 4>(), m);
      break;
    }
  };
  if (std::get<ScalarValue>(node.param(3)).bvalue)
    disp(std::bool_constant<true>());
  else
    disp(std::bool_constant<false>());
  pipe.popOutput(threadGroupId);
  finish(node, pipe, threadGroupId);
}

} // namespace terra::HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(checkerBoard);
HWY_EXPORT(sin);
HWY_EXPORT(distance);
HWY_EXPORT(curve);
HWY_EXPORT(imageMask);

uint32_t lanes();

void constant(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto& out = pipe.getOutput(threadGroupId, lanes());
  out.fill(((ConstantNode&)node).value);
}

void Basics_hwy()
{
  auto builder = buildMeta<NodeMeta_hwy>("@Basics"_ls, "basics");

  {
    builder.add<ConstantNode>(NoDomain(), "@constant", IconBasicChecker);
    builder.fn(constant);
    builder.param<&ConstantNode::value>("@value");
    builder.done();
  }

  {
    builder.add<CheckerNode>("@checkerBoard", IconBasicChecker);
    builder.fn(HWY_DYNAMIC_DISPATCH(checkerBoard));
    builder.param<&CheckerNode::size>("@source");
    builder.done();
  }

  {
    builder.add<SinNode>("@sin", IconBasicSin);
    builder.fn(HWY_DYNAMIC_DISPATCH(sin));
    builder.param<&SinNode::amplitude>("@amplitude");
    builder.param<&SinNode::phase>("@phase");
    builder.done();
  }

  {
    builder.add<DistanceNode>("@distance", IconBasicDistance);
    builder.fn(HWY_DYNAMIC_DISPATCH(distance));
    builder.param<&DistanceNode::fromX>("@fromX");
    builder.param<&DistanceNode::fromY>("@fromY");
    builder.param<&DistanceNode::modulateByFreq>("@modulateByFreq");
    builder.param<&DistanceNode::distanceType>(
      "@distanceType", FmtEnum(0, {"@eucledian", "@eucledianSquared", "@manhattan", "@hybrid", "@maxAxis"}));
    builder.done();
  }

  {
    builder.add<MaskNode>("@imageMask", IconBasicMask);
    builder.fn(HWY_DYNAMIC_DISPATCH(imageMask));
    builder.param<&MaskNode::source>("@source", FmtVal<DataType::eImage>());
    builder.param<&MaskNode::sampler>("@sampling", FmtEnum(0, {"@x1", "@x2", "@x3", "@x4"}));
    builder.param<&MaskNode::offset>("@offset");
    builder.param<&MaskNode::scale>("@scale");
    builder.done();
  }

  {
    builder.add<CurveNode>("@curve", IconBasicCurve);
    builder.fn(HWY_DYNAMIC_DISPATCH(curve));
    builder.param<&CurveNode::source>("@source", FmtVal<DataType::eCurveData>());
    builder.param<&CurveNode::applyX>("@applyX");
    builder.param<&CurveNode::applyY>("@applyY");
    builder.param<&CurveNode::strength>("@strength");
    builder.done();
  }
}

} // namespace terra

#endif
