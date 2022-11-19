
#pragma once

#include "Node.h"
#include "acl/dynamic_array.hpp"

namespace terra
{

struct PostProcessNode : public Node
{
  PostProcessNode(NodeMeta const& m) : Node(m) {}
  Source source;
};

struct ErosionNode : public PostProcessNode
{
  ErosionNode(NodeMeta const& m) : PostProcessNode(m) {}
  int     iteration       = 1000;
  float   density         = 1.f;
  float   evaporationRate = 0.001f;
  float   depositRate     = 0.1f;
  float   minVolume       = 0.01f;
  float   friction        = 0.05f;
  Unorm   effectRadius    = 0.5f;
  int32_t minParticles    = 5;
  vec2    relativePos     = {0.5f, 0.5f};
};

struct Particle
{
  ivec2 pos      = {0, 0};
  vec2  velocity = {0.0f, 0.0f};
  float volume   = 1.0f;
  float sediment = 0.0f;
  float deposit  = 0.0f;
  bool  dead     = false;
};

struct ErosionTileData
{
  std::vector<Particle> particles;
  ivec2                 min;
  ivec2                 max;

  inline bool isInBounds(int x, int y) const
  {
    if (x <= min[0])
      return false;
    if (x >= max[0])
      return false;
    if (y <= min[1])
      return false;
    if (y >= max[1])
      return false;
    return true;
  }
};

struct ErosionCacheData
{
  acl::dynamic_array<ErosionTileData> data;
  uint64_t                            seed = 0;
};

} // namespace terra