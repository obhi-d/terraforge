
#pragma once

#include "Node.h"
#include "hwy/Buffer_hwy.h"

namespace terra
{

struct DerivFractal
{
  float          amp;
  float          freq;
  int32_t        seed;
  hwybuffer_list sum;
  hwybuffer_list dx;
};

struct UnaryNode : public Node
{
  UnaryNode(NodeMeta const& m) : Node(m) {}
  Parameter source;
};

struct BinaryNode : public UnaryNode
{
  BinaryNode(NodeMeta const& m) : UnaryNode(m) {}
  Parameter source2;
};

struct SmoothingNode : public BinaryNode
{
  SmoothingNode(NodeMeta const& m) : BinaryNode(m) {}
  Parameter smoothing;
};

struct MulAddNode : public BinaryNode
{
  MulAddNode(NodeMeta const& m) : BinaryNode(m) {}
  Parameter add;
};

struct BlendNode : public BinaryNode
{
  BlendNode(NodeMeta const& m) : BinaryNode(m) {}
  Parameter factor;
};

struct FalloffNode : public UnaryNode
{
  FalloffNode(NodeMeta const& m) : UnaryNode(m) {}
  Snorm level   = 0.0f;
  vec2  falloff = {0.1f, 0.1f};
  bool  px      = false;
  bool  nx      = false;
  bool  py      = false;
  bool  ny      = false;
};

} // namespace terra