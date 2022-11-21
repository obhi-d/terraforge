
#pragma once

#include "Node.h"
#include "hwy/Buffer_hwy.h"

namespace terra
{

struct ConstantNode : public Node
{
  ConstantNode(NodeMeta const& m) : Node(m) {}
  float value = {};
};

struct CheckerNode : public Node
{
  CheckerNode(NodeMeta const& m) : Node(m) {}
  float size = {};
};

struct SinNode : public Node
{
  SinNode(NodeMeta const& m) : Node(m) {}
  vec2  amplitude = {};
  Angle phase;
};

struct DistanceNode : public Node
{
  DistanceNode(NodeMeta const& m) : Node(m) {}
  Source fromX;
  Source fromY;
  int    distanceType   = 0;
  bool   modulateByFreq = true;
};

struct MaskNode : public Node
{
  MaskNode(NodeMeta const& m) : Node(m) {}
  Source source;
  int    sampler = 0;
  vec2   offset  = {};
  vec2   scale   = {};
};

struct CurveNode : public Node
{
  CurveNode(NodeMeta const& m) : Node(m) {}
  Source source;
  vec2   strength = {};
  bool   applyX   = false;
  bool   applyY   = false;
};

} // namespace terra