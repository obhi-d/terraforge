
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Operators_hwy.cpp"

#include <hwy/foreach_target.h>

#include "Icons.h"
#include "Node.h"
#include "Terra.h"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Operators_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/Utility_hwy.h"

#include "wyrand.h"
#include <hwy/contrib/math/math-inl.h>
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;
using T      = float;

void abs(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  UnaryNode&               node = (UnaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);
  auto& out_a = pipe.getOutput(threadGroupId, lanes);

  auto out_a_data = out_a.data();
  for (uint32_t i = 0; i < out_a.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    hn::Store(hn::Abs(a), d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
}

void flipSign(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  UnaryNode&               node = (UnaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);
  auto& out_a = pipe.getOutput(threadGroupId, lanes);

  auto out_a_data = out_a.data();
  for (uint32_t i = 0; i < out_a.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    hn::Store(hn::Neg(a), d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
}

void add(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Add(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void sub(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Sub(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void mul(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Mul(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void div(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Div(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void min(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Min(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void max(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Max(a, b);
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void minSmooth(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (SmoothingNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.smoothing, pipe, threadGroupId, lanes);

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

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void maxSmooth(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (SmoothingNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.smoothing, pipe, threadGroupId, lanes);

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

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void madd(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (MulAddNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.add, pipe, threadGroupId, lanes);

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

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void pow(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BinaryNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);

  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);

  auto out_a_data = out_a.data();
  auto out_b_data = out_b.data();
  for (uint32_t i = 0; i < out_b.size(); i += lanes)
  {
    const auto a = hn::Load(d, out_a_data + i);
    const auto b = hn::Load(d, out_b_data + i);
    auto const r = hn::Exp(d, hn::Mul(a, hn::Log(d, b)));
    hn::Store(r, d, out_a_data + i);
  }

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
}

void blend(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto&                    node = (BlendNode&)inode;
  const hn::ScalableTag<T> d;
  const auto               lanes = (uint32)hn::Lanes(d);
  modifyDomain(node, pipe, threadGroupId);

  auto const oneSixth = hn::Set(d, 1.f / 6.f);
  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);

  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  auto& out_b = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.source2, pipe, threadGroupId, lanes);
  auto& out_c = pipe.pushOutput(threadGroupId, lanes);
  NodeMeta_hwy::write(node.factor, pipe, threadGroupId, lanes);

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

  finish(node, pipe, threadGroupId);
  pipe.popOutput(threadGroupId);
  pipe.popOutput(threadGroupId);
}

void falloff(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  const I_t itag{};

  auto&      node  = (FalloffNode&)inode;
  const auto lanes = (uint32)hn::Lanes(vtag);
  modifyDomain(node, pipe, threadGroupId);

  NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes);
  auto& out_a = pipe.getOutput(threadGroupId, lanes);

  auto  out_a_data = out_a.data();
  auto& inp        = pipe.getInput(threadGroupId, lanes, true);
  auto  inp_x_data = inp.x.data();
  auto  inp_y_data = inp.y.data();

  auto        freq     = pipe.frequency(threadGroupId);
  auto const& td       = pipe.getThreadData(threadGroupId);
  auto        centerx  = hn::Set(vtag, ((float)td.params.startxy[0] + ((float)td.params.tileSize[0] * .5f)) * freq);
  auto        centery  = hn::Set(vtag, ((float)td.params.startxy[1] + ((float)td.params.tileSize[1] * .5f)) * freq);
  auto        recipx   = hn::Set(vtag, 1.f / (((float)td.params.tileSize[0] * .5f) * freq));
  auto        recipy   = hn::Set(vtag, 1.f / (((float)td.params.tileSize[1] * .5f) * freq));
  auto        edge     = hn::Set(vtag, node.level);
  auto        falloffX = hn::Set(vtag, node.falloff.x);
  auto        falloffY = hn::Set(vtag, node.falloff.y);
  
  for (uint32_t ii = 0; ii < out_a.size(); ii += lanes)
  {
    const auto x  = hn::Load(vtag, inp_x_data + ii);
    const auto y  = hn::Load(vtag, inp_y_data + ii);
    auto       z  = hn::Load(vtag, out_a_data + ii) - edge;
    auto       dx = x - centerx;
    auto       dy = y - centery;
    auto       fx = FS_Pow_f32(hn::Abs(dx) * recipx, falloffX);
    auto       fy = FS_Pow_f32(hn::Abs(dy) * recipy, falloffY);
    // const auto a = hn::Load(vtag, out_a_data + i);
    // hn::Store(hn::Abs(a), d, out_a_data + i);
    auto dist = hn::Zero(vtag);
    if (node.nx)
      dist += hn::IfNegativeThenElse(dx, fx, hn::Zero(vtag));
    if (node.px)
      dist += hn::IfNegativeThenElse(dx, hn::Zero(vtag), fx);
    if (node.ny)
      dist += hn::IfNegativeThenElse(dy, fy, hn::Zero(vtag));
    if (node.py)
      dist += hn::IfNegativeThenElse(dy, hn::Zero(vtag), fy);

    dist = hn::Sqrt(dist);
    z = hn::IfThenElse(dist < float32v(1.f), (z - z * (dist * dist * (float32v(3.f) - float32v(2.f) * dist))) + edge,
                        edge);
    hn::Store(z, vtag, out_a_data + ii);
  }
  
  finish(node, pipe, threadGroupId);
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
HWY_EXPORT(flipSign);
HWY_EXPORT(abs);
HWY_EXPORT(falloff);
void Operators_hwy()
{

  auto builder = buildMeta<NodeMeta_hwy>("@Operators"_ls, "operators");

  // Common
  {
    builder.add<UnaryNode>(NoDomain(), "@abs", IconOpAbs);
    builder.fn(HWY_DYNAMIC_DISPATCH(abs));
    builder.param<&UnaryNode::source>("@source");
    builder.done();
  }

  {
    builder.add<UnaryNode>(NoDomain(), "@flipSign", IconOpFlipSign);
    builder.fn(HWY_DYNAMIC_DISPATCH(flipSign));
    builder.param<&UnaryNode::source>("@source");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@add", IconOpAdd);
    builder.fn(HWY_DYNAMIC_DISPATCH(add));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@sub", IconOpSub);
    builder.fn(HWY_DYNAMIC_DISPATCH(sub));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@mul", IconOpMul);
    builder.fn(HWY_DYNAMIC_DISPATCH(mul));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@div", IconOpDiv);
    builder.fn(HWY_DYNAMIC_DISPATCH(div));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@pow", IconOpPow);
    builder.fn(HWY_DYNAMIC_DISPATCH(pow));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@min", IconOpMin);
    builder.fn(HWY_DYNAMIC_DISPATCH(min));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<BinaryNode>(NoDomain(), "@max", IconOpMax);
    builder.fn(HWY_DYNAMIC_DISPATCH(max));
    builder.param<&BinaryNode::source>("@source");
    builder.param<&BinaryNode::source2>("@source2");
    builder.done();
  }

  {
    builder.add<SmoothingNode>(NoDomain(), "@minSmooth", IconOpMinSmooth);
    builder.fn(HWY_DYNAMIC_DISPATCH(minSmooth));
    builder.param<&SmoothingNode::source>("@source");
    builder.param<&SmoothingNode::source2>("@source2");
    builder.param<&SmoothingNode::smoothing>("@smoothing");
    builder.done();
  }

  {
    builder.add<SmoothingNode>(NoDomain(), "@maxSmooth", IconOpMaxSmooth);
    builder.fn(HWY_DYNAMIC_DISPATCH(maxSmooth));
    builder.param<&SmoothingNode::source>("@source");
    builder.param<&SmoothingNode::source2>("@source2");
    builder.param<&SmoothingNode::smoothing>("@smoothing");
    builder.done();
  }

  {
    builder.add<MulAddNode>(NoDomain(), "@mulAdd", IconOpMulAdd);
    builder.fn(HWY_DYNAMIC_DISPATCH(madd));
    builder.param<&MulAddNode::source>("@source");
    builder.param<&MulAddNode::source2>("@mul");
    builder.param<&MulAddNode::add>("@add");
    builder.done();
  }

  {
    builder.add<BlendNode>(NoDomain(), "@blend", IconOpBlend);
    builder.fn(HWY_DYNAMIC_DISPATCH(blend));
    builder.param<&BlendNode::source>("@source");
    builder.param<&BlendNode::source2>("@source2");
    builder.param<&BlendNode::factor>("@factor");
    builder.done();
  }

  {
    builder.add<FalloffNode>("@falloff", IconOpFalloff);
    builder.fn(HWY_DYNAMIC_DISPATCH(falloff));
    builder.param<&FalloffNode::source>("@source");
    builder.param<&FalloffNode::level>("@level");
    builder.param<&FalloffNode::falloff>("@level");
    builder.param<&FalloffNode::px>("@+x");
    builder.param<&FalloffNode::nx>("@-x");
    builder.param<&FalloffNode::py>("@+y");
    builder.param<&FalloffNode::ny>("@-y");
    builder.done();
  }
}

} // namespace terra

#endif
