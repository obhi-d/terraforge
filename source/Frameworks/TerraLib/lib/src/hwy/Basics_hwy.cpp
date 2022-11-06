
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Basics_hwy.cpp"

#include "Common.h"
#include "CurveData.h"
#include "Image.h"
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

void checkerBoard(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data   = out.data();
  auto inp_x_data = inp.x.data();
  auto inp_y_data = inp.y.data();
  auto multiplier = hn::Set(vtag, std::get<ScalarValue>(node.param(0)).value);
  for (uint32_t ii = 0; ii < out.size(); ii += lanes)
  {
    const auto x = hn::Load(vtag, inp_x_data + ii);
    const auto y = hn::Load(vtag, inp_y_data + ii);

    auto value = hn::Xor(FS_Convertf32_i32(x * multiplier), FS_Convertf32_i32(y * multiplier));

    hn::Store(float32v(1.0f) * FS_Casti32_f32(hn::ShiftLeft<31>(value)), vtag, out_data + ii);
  }
}

void curve(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto         out_data = out.data();
  const float* xy[2]    = {inp.x.data(), inp.y.data()};
  auto const&  param    = node.param(0);
  // if we dont have a curve, we just fill with the constant

  if (std::holds_alternative<Source>(param))
  {
    auto const& td     = pipe.getThreadData(threadGroupId);
    auto&       cd     = get().get<CurveData>(std::get<Source>(param).source);
    bool        allowX = std::get<ScalarValue>(node.param(1)).bvalue;
    bool        allowY = std::get<ScalarValue>(node.param(2)).bvalue;
    auto const& str    = std::get<ScalarValue>(node.param(3)).value2;

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
          const auto x = hn::Load(vtag, xy[0] + ii);
          auto       u = FS_SubMul(x, xoffset, xrsize);
          hn::Vec<V_t> storeV = hn::Set(vtag, 0);
          for (uint32_t l = 0; l < lanes; ++l)
            storeV = hn::InsertLane(storeV, l, cd.spline(hn::ExtractLane(u, l)));
          store = hn::Mul(storeV, xstrength);
        }
        if constexpr (applyY)
        {
          const auto y = hn::Load(vtag, xy[1] + ii);
          auto       u = FS_SubMul(y, yoffset, yrsize);
          auto       storeX = hn::Set(vtag, 0);
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
}

void imageMask(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto         out_data = out.data();
  const float* xy[2]    = {inp.x.data(), inp.y.data()};
  auto const&  param    = node.param(0);
  // if we dont have a curve, we just fill with the constant

  if (std::holds_alternative<Source>(param))
  {
    auto const& td       = pipe.getThreadData(threadGroupId);
    auto&       cd       = get().get<Image>(std::get<Source>(param).source);
    int         sampling = std::get<ScalarValue>(node.param(1)).ivalue;
    auto        offset   = std::get<ScalarValue>(node.param(2)).value2;
    auto        scale    = std::get<ScalarValue>(node.param(3)).value2;
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
}

void sin(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& out = pipe.getOutput(threadGroupId, lanes);
  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto out_data = out.data();

  auto const& amplitude  = std::get<ScalarValue>(node.param(0));
  auto const& phase      = std::get<ScalarValue>(node.param(1));
  auto        ampX       = hn::Set(vtag, amplitude.value2[0]);
  auto        ampY       = hn::Set(vtag, amplitude.value2[1]);
  auto        phaseX     = hn::Set(vtag, phase.value2[0]);
  auto        phaseY     = hn::Set(vtag, phase.value2[1]);
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
}

void distance(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t  vtag{};
  const I_t  itag{};
  const auto lanes = (uint32)hn::Lanes(vtag);

  auto& inp = pipe.getInput(threadGroupId, lanes, true);

  auto& out_x = pipe.getOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_y = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto        out_x_data = out_x.data();
  auto        out_y_data = out_y.data();
  auto const& from       = std::get<ScalarValue>(node.param(0));
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
    switch (std::get<ScalarValue>(node.param(3)).ivalue)
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
  if (std::get<ScalarValue>(node.param(2)).bvalue)
    disp(std::bool_constant<true>());
  else
    disp(std::bool_constant<false>());
  pipe.popOutput(threadGroupId);
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
void Basics_hwy()
{
  // Common
  NodeMeta_hwy meta;
  meta.category = "@Basics"_ls;
  meta.style    = "basics";

  // checkerBoard
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat>(), "@cbsize");
  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(checkerBoard);
  get().addMeta("@checkerBoard", meta);
  meta.parameterDef.clear();

  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(), "@amplitude");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(0.0f, -consts::pi, consts::pi), "@phase");
  meta.icon = "\xef\x87\xbe";
  meta.fn   = HWY_DYNAMIC_DISPATCH(sin);
  get().addMeta("@sin", meta);
  meta.parameterDef.clear();

  meta.parameterDef.emplace_back(
    FmtVal<DataType::eBuffer>(0.0f, std::numeric_limits<float>::min(), std::numeric_limits<float>::max()), "@fromX");
  meta.parameterDef.emplace_back(
    FmtVal<DataType::eBuffer>(0.0f, std::numeric_limits<float>::min(), std::numeric_limits<float>::max()), "@fromY");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBool>(1), "@modulateByFreq");
  meta.parameterDef.emplace_back(
    FmtEnum(0, {"@eucledian"_ls, "@eucledianSquared"_ls, "@manhattan"_ls, "@hybrid"_ls, "@maxAxis"_ls}),
    "@distanceType");
  meta.icon                  = "\xef\x87\xbe";
  meta.fn                    = HWY_DYNAMIC_DISPATCH(distance);
  meta.attribTileConstrained = true;
  get().addMeta("@distance", meta);
  meta.parameterDef.clear();

  meta.parameterDef.emplace_back(FmtVal<DataType::eImage>(), "@mask");
  meta.parameterDef.emplace_back(FmtEnum(0, {"@x1"_ls, "@x2"_ls, "@x3"_ls, "@x4"_ls}), "@sampling");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(), "@offset");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(), "@scale");
  meta.icon                  = "\xef\x87\xbe";
  meta.fn                    = HWY_DYNAMIC_DISPATCH(imageMask);
  meta.attribTileConstrained = true;
  get().addMeta("@imageMask", meta);
  meta.parameterDef.clear();
  meta.attribTileConstrained = false;

  meta.parameterDef.emplace_back(FmtVal<DataType::eCurveData>(), "@curve");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBool>(), "@applyX");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBool>(), "@applyY");
  meta.parameterDef.emplace_back(FmtVal<DataType::eFloat2>(), "@strength");
  meta.icon                  = "\xef\x87\xbe";
  meta.fn                    = HWY_DYNAMIC_DISPATCH(curve);
  meta.attribTileConstrained = true;
  get().addMeta("@curve", meta);
  meta.parameterDef.clear();
  meta.attribTileConstrained = false;
}

} // namespace terra

#endif
