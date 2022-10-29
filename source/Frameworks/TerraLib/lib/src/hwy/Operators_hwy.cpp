
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Operators_hwy.cpp"

#include <hwy/foreach_target.h>

#include "NodeMeta.h"
#include "Terra.h"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"

#include "metas/Operators.h"

#include <hwy/contrib/math/math-inl.h>
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;
using T      = float;

void add(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeBiop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);

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

void sub(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeBiop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);

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

void mul(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeBiop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);

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

void div(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeBiop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);

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

void madd(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeTriop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.opC, pipe, threadGroupId, lanes);

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

void pow(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  auto&                    node  = (NodeBiop&)inode;

  NodeMeta_hwy::write(node.opA, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.opB, pipe, threadGroupId, lanes);

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

} // namespace terra::HWY_NAMESPACE

#if HWY_ONCE

namespace terra
{
HWY_EXPORT(add);
HWY_EXPORT(sub);
HWY_EXPORT(mul);
HWY_EXPORT(div);
HWY_EXPORT(madd);
HWY_EXPORT(pow);
void Operators_hwy()
{
  // Common
  NodeMeta_hwy meta;
  meta.category = "@Operators"_ls;
  meta.style    = "Operators";

  // Binary
  meta.parameterDef.emplace_back(MemberPtr<&NodeBiop::opA>{}, FmtVal<DataType::eBuffer>(), "@OpA"_ls, "@OpAHelp"_ls,
                                 "@OpATooltip"_ls);

  meta.parameterDef.emplace_back(MemberPtr<&NodeBiop::opB>{}, FmtVal<DataType::eBuffer>(), "@OpB"_ls, "@OpBHelp"_ls,
                                 "@OpBTooltip"_ls);
  meta.create = [](NodeMeta const& meta) -> std::shared_ptr<Node>
  {
    return std::make_shared<NodeBiop>(meta);
  };

  // Add
  meta.icon    = u8"\xef\x81\x95";
  meta.name    = "@Add"_ls;
  meta.tooltip = "@AddTooltip"_ls;
  meta.help    = "@AddHelp"_ls;
  meta.fn      = HWY_DYNAMIC_DISPATCH(add);
  get().addMeta("add", meta);

  // Sub
  meta.icon     = u8"\xef\x81\x96";
  meta.name     = "@Sub"_ls;
  meta.category = "@Operators"_ls;
  meta.tooltip  = "@SubTooltip"_ls;
  meta.help     = "@SubHelp"_ls;
  meta.fn       = HWY_DYNAMIC_DISPATCH(sub);
  get().addMeta("sub", meta);

  // Mul
  meta.icon     = u8"\xef\x81\x97";
  meta.name     = "@Mul"_ls;
  meta.category = "@Operators"_ls;
  meta.tooltip  = "@MulTooltip"_ls;
  meta.help     = "@MulHelp"_ls;
  meta.fn       = HWY_DYNAMIC_DISPATCH(mul);
  get().addMeta("mul", meta);

  // Divide
  meta.icon     = u8"\xef\x94\xa9";
  meta.name     = "@Div"_ls;
  meta.category = "@Operators"_ls;
  meta.tooltip  = "@DivTooltip"_ls;
  meta.help     = "@DivHelp"_ls;
  meta.fn       = HWY_DYNAMIC_DISPATCH(div);
  get().addMeta("div", meta);

  // Pow
  meta.icon     = u8"\xef\x84\xab";
  meta.name     = "@Pow"_ls;
  meta.category = "@Operators"_ls;
  meta.tooltip  = "@PowTooltip"_ls;
  meta.help     = "@PowHelp"_ls;
  meta.fn       = HWY_DYNAMIC_DISPATCH(pow);
  get().addMeta("pow", meta);

  // Tartiary
  meta.parameterDef.clear();
  meta.parameterDef.emplace_back(MemberPtr<&NodeTriop::opA>{}, FmtVal<DataType::eBuffer>(), "@OpA"_ls, "@OpAHelp"_ls,
                                 "@OpATooltip"_ls);
  meta.parameterDef.emplace_back(MemberPtr<&NodeTriop::opB>{}, FmtVal<DataType::eBuffer>(), "@OpB"_ls, "@OpBHelp"_ls,
                                 "@OpBTooltip"_ls);
  meta.parameterDef.emplace_back(MemberPtr<&NodeTriop::opC>{}, FmtVal<DataType::eBuffer>(), "@OpC"_ls, "@OpCHelp"_ls,
                                 "@OpCTooltip"_ls);
  meta.create = [](NodeMeta const& meta) -> std::shared_ptr<Node>
  {
    return std::make_shared<NodeTriop>(meta);
  };

  // MAdd
  meta.icon     = u8"\xef\x84\xab";
  meta.name     = "@Madd"_ls;
  meta.category = "@Operators"_ls;
  meta.tooltip  = "@MaddTooltip"_ls;
  meta.help     = "@MaddHelp"_ls;
  meta.fn       = HWY_DYNAMIC_DISPATCH(madd);
  get().addMeta("madd", meta);
}

} // namespace terra

#endif