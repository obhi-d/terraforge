
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
  vec2    relativePos     = {0.5f, 0.5f};
  Unorm   effectRadius    = 0.5f;
  int32_t minParticles    = 5;
  int32_t maxParticles    = 1000;
  int     iteration       = 1000;
  int     lifetime        = 100;
  Unorm   baseInertia     = 0.1f;
  Unorm   inertiaJitter   = 0.01f;
  float   maxCapacity     = 32.1f;
  Unorm   dropletVolume   = 1.0f;
  float   minSlope        = 0.001f;
  Unorm   depositRate     = 0.1f;
  Unorm   erosionRate     = 0.1f;
  float   erodeRadius     = 1.f;
  Unorm   evaporationRate = 0.001f;
  float   gravity         = 9.8f;
  float   minSediment     = 0.0f;
  int32_t randomizer      = 0x5522;
  Source  erosionMask;

  Unorm blurFactor = 0.1f;
  bool  blur       = true;
};

struct Particle
{
  inline static float constexpr minGradient = 0.00001f;
  vec2  prevPos                             = {0, 0};
  vec2  pos                                 = {0.f, 0.f};
  vec2  dir                                 = {0.0f, 0.0f};
  float velocity                            = 0.1f;
  float inertia                             = 0.0f;

  float water    = 1.0f;
  float sediment = 0.0f;
  float deposit  = 0.0f;
  int   life     = 100;
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
  uint64_t                            seed        = 0;
  bool                                erosionMask = false;
  std::vector<ivec2>                  erodeKernel;
  std::vector<float>                  erodeKernelWeights;
  int32_t                             iteration = 0;
};

} // namespace terra