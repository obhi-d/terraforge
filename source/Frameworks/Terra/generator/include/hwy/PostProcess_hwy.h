
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
  int32_t particleCount   = 600000;
  int     iteration       = 21000;
  int     lifetime        = 30;
  Unorm   inertia         = 0.1f;
  float   maxCapacity     = 10.1f;
  float   minCapacity     = 1.f;
  float   minSlope        = 0.0001f;
  Unorm   depositRate     = 1.0f;
  Unorm   erosionRate     = 0.1f;
  float   erodeRadius     = 2.f;
  Unorm   evaporationRate = 0.1f;
  float   gravity         = 4.0f;
  float   minSediment     = 0.0f;
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

  float    water    = 1.0f;
  float    sediment = 0.0f;
  float    deposit  = 0.0f;
  int      age      = 0;
  uint64_t seed     = 3145739;

  Particle() = default;
  Particle(uint64_t seed, int age, vec2 center, float radius, float variation);
};

struct ErosionTileData
{
  std::vector<Particle> particles;
  float                 radius;
  float                 variation;
  vec2                  center;
  vec2                  min;
  vec2                  max;

  inline bool isInBounds(int x, int y) const
  {
    if (x <= min.x)
      return false;
    if (x >= max.x)
      return false;
    if (y <= min.y)
      return false;
    if (y >= max.y)
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