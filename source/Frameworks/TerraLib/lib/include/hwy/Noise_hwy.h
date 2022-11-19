
#pragma once

#include "Node.h"
#include "hwy/Buffer_hwy.h"

namespace terra
{

struct RidgedNoiseNode : public Node
{
  RidgedNoiseNode(NodeMeta const& m) : Node(m) {}
  Parameter ridgedOffset;
  Parameter source;
};

struct WorlyNode : public Node
{
  WorlyNode(NodeMeta const& m) : Node(m) {}
  Parameter falloff;
  Parameter source;
};

struct FlowNode : public Node
{
  FlowNode(NodeMeta const& m) : Node(m) {}
  Angle angle;
};

struct MultiFractalNode : public Node
{
  MultiFractalNode(NodeMeta const& m) : Node(m) {}
  Parameter source;
  int       octaves    = 4;
  float     lacunarity = 2.0f;
  Unorm     gain       = 0.5f;
  int       seedOffset = 1;
};

struct DerivFractalNode : public Node
{
  DerivFractalNode(NodeMeta const& m) : Node(m) {}
  int   octaves    = 4;
  float lacunarity = 2.0f;
  Unorm gain       = 0.5f;
  int   seedOffset = 1;
};

struct CellularValueNode : public Node
{
  CellularValueNode(NodeMeta const& m) : Node(m) {}
  Parameter jitter;
  int       returnType   = 0;
  int       distanceType = 0;
};

struct MultiFractal
{
  float          amp;
  float          freq;
  int32_t        seed;
  hwybuffer_list outputs;
};

struct DerivFractal
{
  float          amp;
  float          freq;
  int32_t        seed;
  hwybuffer_list sum;
  hwybuffer_list dx;
};

} // namespace terra