
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Operators_hwy.cpp"

#include <hwy/foreach_target.h>

#include "Node.h"
#include "Terra.h"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"

#include <hwy/contrib/math/math-inl.h>
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;
using T      = float;

void add(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Add(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void sub(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Sub(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void mul(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Mul(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void div(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Div(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void min(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Min(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void max(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Max(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void minSmooth(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  auto out_c_data = out_c.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    const auto c = hn::Load(d, out_c_data + i);
    const auto h = hn::Max(c - hn::Abs(a - b), hn::Zero(d)) * hn::ApproximateReciprocal(c);
    
    auto const r = hn::NegMulAdd(oneSixth, h * h * h * c, hn::Min(a, b));
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void maxSmooth(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  auto out_c_data = out_c.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    const auto c = hn::Load(d, out_c_data + i);
    const auto h = hn::Max(c - hn::Abs(a - b), hn::Zero(d)) * hn::ApproximateReciprocal(c);

    auto const r = hn::NegMulAdd(oneSixth, h * h * h * c, hn::Max(a, b));
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void madd(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  auto out_c_data = out_c.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    const auto c = hn::Load(d, out_c_data + i);
    auto const r = hn::MulAdd(a, b, c);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void pow(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Exp(d, hn::Mul(a, hn::Log(d, b)));
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
}

void blend(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.param(0), pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(1), pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.param(2), pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  auto out_c_data = out_c.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    const auto c = hn::Load(d, out_c_data + i);
    
    auto const r = hn::MulAdd(a, hn::Set(d, 1.f) - c, b * c);
    hn::Store(r, d, out_a_data + i);
  }

  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

} // namespace terra::HWY_NAMESPACE

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(add);
HWY_EXPORT(sub);
HWY_EXPORT(mul);
HWY_EXPORT(div);
HWY_EXPORT(madd);
HWY_EXPORT(pow);
HWY_EXPORT(min);
HWY_EXPORT(max);
HWY_EXPORT(minSmooth);
HWY_EXPORT(maxSmooth);
HWY_EXPORT(blend);
void Operators_hwy()
{
  // Common
  NodeMeta_hwy meta;
  meta.category = "@Operators"_ls;
  meta.style    = "operators";

  // Binary
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@OpA");
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@OpB");

  // Add
  meta.icon    = "\xef\x81\x95";
  meta.fn      = HWY_DYNAMIC_DISPATCH(add);
  get().addMeta("@add", meta);

  // Sub
  meta.icon     = "\xef\x81\x96";
  meta.fn       = HWY_DYNAMIC_DISPATCH(sub);
  get().addMeta("@sub", meta);

  // Mul
  meta.icon     = "\xef\x81\x97";
  meta.fn       = HWY_DYNAMIC_DISPATCH(mul);
  get().addMeta("@mul", meta);

  // Divide
  meta.icon     = "\xef\x94\xa9";
  meta.fn       = HWY_DYNAMIC_DISPATCH(div);
  get().addMeta("@div", meta);

  // Pow
  meta.icon     = "\xef\x84\xab";
  meta.fn       = HWY_DYNAMIC_DISPATCH(pow);
  get().addMeta("@pow", meta);

  // Min
  meta.icon = "\xef\x84\xab";
  meta.fn   = HWY_DYNAMIC_DISPATCH(min);
  get().addMeta("@min", meta);

  // Max
  meta.icon = "\xef\x84\xab";
  meta.fn   = HWY_DYNAMIC_DISPATCH(max);
  get().addMeta("@min", meta);

  // Tartiary
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(.1f, std::numeric_limits<float>::epsilon(), 1.f), "@Smoothing");

  // MinSmooth
  meta.icon = "\xef\x84\xab";
  meta.fn   = HWY_DYNAMIC_DISPATCH(minSmooth);
  get().addMeta("@minSmooth", meta);

  // MaxSmooth
  meta.icon = "\xef\x84\xab";
  meta.fn   = HWY_DYNAMIC_DISPATCH(maxSmooth);
  get().addMeta("@minSmooth", meta);

  // Tartiary
  meta.parameterDef.pop_back();
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@OpC");

  // MAdd
  meta.icon     = "\xef\x81\x97";
  meta.fn       = HWY_DYNAMIC_DISPATCH(madd);
  get().addMeta("@madd", meta);

  meta.parameterDef.pop_back();
  meta.parameterDef.emplace_back(FmtVal<DataType::eBuffer>(), "@Blend");

  // MAdd
  meta.icon = "\xef\x81\x97";
  meta.fn   = HWY_DYNAMIC_DISPATCH(blend);
  get().addMeta("@blend", meta);
}

} // namespace terra

#endif
