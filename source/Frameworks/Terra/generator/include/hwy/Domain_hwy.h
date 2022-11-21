
#pragma once

#include "Node.h"
#include "hwy/Buffer_hwy.h"

namespace terra
{

struct DomainRotateNode : public Node
{
  DomainRotateNode(NodeMeta const& m) : Node(m) {}
  Angle angle = {};
};

struct DomainScaleOffsetNode : public Node
{
  DomainScaleOffsetNode(NodeMeta const& m) : Node(m) {}
  vec2 scale  = {1.f, 1.f};
  vec2 offset = {};
};

struct DomainWarpNode : public Node
{
  DomainWarpNode(NodeMeta const& m) : Node(m) {}
  int   octaves    = 4;
  float lacunarity = 2.0f;
  Unorm gain       = 0.5f;
  int   seedOffset = 1;
};

struct DomainWarpBoundedNode : public DomainWarpNode
{
  DomainWarpBoundedNode(NodeMeta const& m) : DomainWarpNode(m) {}
  float strength = 0.5f;
  float bounds   = {};
};

struct OutputToDomain : public Node
{
  OutputToDomain(NodeMeta const& m) : Node(m) {}
  Source source;
  Source source2;
};

struct DomainFractal
{
  float      amp  = {};
  float      freq = {};
  int32_t    seed = {};
  hwyvb_list inputs;
};

} // namespace terra