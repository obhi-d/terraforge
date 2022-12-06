#include "Icons.h"
#include "Node.h"
#include "Sampler2D.h"
#include "Terra.h"

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/PostProcess_hwy.h"
#include "hwy/Utility_hwy.h"

#include "wyrand.h"

namespace terra
{
uint32_t lanes();

Particle::Particle(uint64_t sseed, int startAge, vec2 center, float range, float variation)
{
  seed         = sseed;
  float radius = wysnorm(&seed) * range + wysnorm(&seed) * variation;
  float angle  = wysnorm(&seed) * consts::pi;
  pos.x        = center.x + radius * std::sin(angle);
  pos.y        = center.y + radius * std::cos(angle);
  prevPos      = pos;
  velocity     = 0.f;
  water        = 1.f;
  age          = startAge;
  deposit      = 0.f;
  dir          = {0.0f, 0.0f};
  sediment     = 0.0f;
}

void postProcessPassthrough(Node& inode, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto& node = (PostProcessNode&)inode;
  if (!pipe.wasReissued(inode.getSelf()))
    NodeMeta_hwy::write(node.source, pipe, threadGroupId, lanes());
}

void hydraulicErosion_prepare(Node& inode, Pipeline_hwy& pipe)
{
  auto& node = (ErosionNode&)inode;
  // spawn particles
  uint32_t numTiles = pipe.getNumTiles();
  auto&    data     = pipe.addCacheData<ErosionCacheData>(node.getSelf());
  data.seed         = pipe.params().seed;

  float erodeSizeX = std::max(1.f, node.erodeRadius);
  float erodeSizeY = std::max(1.f, node.erodeRadius);
  float rad        = std::sqrt((erodeSizeX * erodeSizeX + erodeSizeY * erodeSizeY));
  data.erodeKernel.emplace_back(ivec2{0, 0});
  for (int x = 1, endX = (int)erodeSizeX; x <= endX; ++x)
  {
    for (int y = 1, endY = (int)erodeSizeY; y <= endY; ++y)
    {
      if (std::sqrt((float)(x * x + y * y)) <= rad)
      {
        data.erodeKernel.emplace_back(ivec2{x, y});
        data.erodeKernel.emplace_back(ivec2{-x, y});
        data.erodeKernel.emplace_back(ivec2{x, -y});
        data.erodeKernel.emplace_back(ivec2{-x, -y});
      }
    }
  }
  data.erodeKernel.shrink_to_fit();
  data.erodeKernelWeights.resize(data.erodeKernel.size());
  if (numTiles > 0)
  {
    data.data = acl::dynamic_array<ErosionTileData>(numTiles);
    for (uint32_t i = 0; i < numTiles; ++i)
    {
      auto&    tile   = pipe.getTileData(i);
      auto&    ed     = data.data[i];
      uint32_t nbPart = (((uint64_t)node.particleCount) * (uint64_t)node.lifetime) / node.iteration;
      // DEBUG TODO
      ed.radius =
        std::sqrt((node.effectRadius * tile.params.tileSize.x) * (node.effectRadius * tile.params.tileSize.y) * .25f);
      ed.variation = node.effectRadius * std::max(tile.params.tileSize.x, tile.params.tileSize.y) * .05f;
      ed.center.x  = node.relativePos.x * tile.params.tileSize.x;
      ed.center.y  = node.relativePos.y * tile.params.tileSize.y;
      ed.min       = ed.center - glm::vec2(ed.radius);
      ed.max       = ed.center + glm::vec2(ed.radius);
      ed.particles.resize(nbPart);
      for (auto& p : ed.particles)
      {
        p.seed = wyrand(&data.seed);
        p      = Particle(p.seed, node.lifetime, ed.center, ed.radius, ed.variation);
      }
      // select kernel
    }
    data.erosionMask = (node.erosionMask.source && DataSource::isValid(node.erosionMask.source));
  }
}

void hydraulicErosion_end(Node& inode, Pipeline_hwy& pipe)
{
  auto& node = (ErosionNode&)inode;
  // only work on it if we can reissue the node
  auto& data = pipe.getCacheData<ErosionCacheData>(node.getSelf());
  if (data.iteration < node.iteration)
  {
    if (!pipe.reissue(inode.getSelf()))
      return;
  }
  else if (pipe.wasReissued(inode.getSelf()))
  {
    pipe.resetLastIssued();
    return;
  }
  data.iteration++;

  // spawn particles
  uint32_t numTiles = pipe.getNumTiles();

  for (uint32_t tileIdx = 0; tileIdx < numTiles; ++tileIdx)
  {
    auto& tile = pipe.getTileData(tileIdx);
    auto& ed   = data.data[tileIdx];

    auto sampler = Sampler2D(tile.buffer.data(), tile.params.tileSize[0], tile.params.tileSize[1]);
    auto respawn = [&](Particle& particle)
    {
      particle = Particle(particle.seed, node.lifetime, ed.center, ed.radius, ed.variation);
    };

    get().pool().for_each(
      ed.particles.begin(), ed.particles.end(), 8,
      [&](Particle& p)
      {
        uint32_t ix = (uint32_t)p.pos.x;
        uint32_t iy = (uint32_t)p.pos.y;
        if (!(p.age--) || p.water < std::numeric_limits<float>::epsilon())
        {
          respawn(p);
          return;
        }

        auto g    = sampler.heightGradientAt(p.pos);
        auto ndir = sub(scale(node.inertia, p.dir), scale(1.f - node.inertia, g));
        if (ndir.x == 0.f && ndir.y == 0.f)
        {
          // give it random direction
          float angle = wysnorm(&p.seed) * consts::pi;
          ndir.x      = std::sin(angle);
          ndir.y      = std::cos(angle);
        }
        p.dir     = glm::normalize(ndir);
        p.prevPos = p.pos;
        p.pos += p.dir;
        ix = (uint32_t)p.pos.x;
        iy = (uint32_t)p.pos.y;
        if (!sampler.isInBounds(ix, iy))
        {
          respawn(p);
          return;
        }
        float hold  = sampler.bisample(p.prevPos.x, p.prevPos.y);
        float hnew  = sampler.bisample(p.pos.x, p.pos.y);
        float hdiff = hnew - hold;
        if (hdiff > 0.f)
        {
          p.deposit = std::min(hdiff, p.sediment);
          p.sediment -= p.deposit;
        }
        else
        {
          float capacity =
            std::max(std::max(-hdiff, node.minSlope) * p.velocity * p.water * node.maxCapacity, node.minCapacity);
          if (p.sediment > capacity)
          {
            p.deposit = (p.sediment - capacity) * node.depositRate;
            p.sediment -= p.deposit;
          }
          else
          {
            p.deposit = std::min((capacity - p.sediment) * node.erosionRate, -hdiff);
            if (data.erosionMask)
            {
              auto const& image = get().get<Image>(node.erosionMask.source);
              p.deposit *= image.sample(p.pos.x / tile.params.tileSize.x, p.pos.y / tile.params.tileSize.y);
            }
            p.sediment += p.deposit;
            p.deposit = -p.deposit;
          }
        }

        p.velocity = std::sqrt(p.velocity * p.velocity + std::abs(hdiff) * node.gravity);
        p.water *= (1.f - node.evaporationRate);
        assert(p.sediment >= 0.f);
      });

    for (auto& p : ed.particles)
    {
      if (p.deposit < 0.f)
      {

        float weightSum = 0.f;
        for (uint32 ek = 0; ek < (uint32)data.erodeKernel.size(); ++ek)
        {
          auto const k  = data.erodeKernel[ek];
          int        sx = (int)p.prevPos.x + k.x;
          int        sy = (int)p.prevPos.y + k.y;
          if (sampler.isInBounds(sx, sy))
          {
            data.erodeKernelWeights[ek] =
              std::max(0.f, node.erodeRadius - std::sqrt(distanceSq(p.prevPos, vec2{(float)sx, (float)sy})));
            weightSum += data.erodeKernelWeights[ek];
          }
        }
        if (weightSum > 0.f)
        {
          weightSum = 1.f / weightSum;
          for (uint32 ek = 0; ek < (uint32)data.erodeKernel.size(); ++ek)
          {
            auto const k  = data.erodeKernel[ek];
            int        sx = (int)p.prevPos.x + k.x;
            int        sy = (int)p.prevPos.y + k.y;
            if (sampler.isInBounds(sx, sy))
              sampler.add(p.deposit * data.erodeKernelWeights[ek] * weightSum, sx, sy);
          }
        }
      }
      else
      {
        int   px = (int)std::floor(p.prevPos.x);
        int   py = (int)std::floor(p.prevPos.y);
        float u  = (p.prevPos.x - std::floor(p.prevPos.x));
        float v  = (p.prevPos.y - std::floor(p.prevPos.y));

        sampler.add((p.deposit * (1 - u) * (1 - v)), px, py);
        sampler.add((p.deposit * u * (1 - v)), px + 1, py);
        sampler.add((p.deposit * (1 - u) * v), px, py + 1);
        sampler.add((p.deposit * u * v), px + 1, py + 1);
      }
    }

    if (node.blur && data.iteration == node.iteration)
    {
      auto unblurred    = tile.buffer;
      auto constSampler = Sampler2D(unblurred.data(), tile.params.tileSize[0], tile.params.tileSize[1]);
      get().pool().for_each(0, tile.params.tileSize[1],
                            [&, factor = node.blurFactor](int y)
                            {
                              float blur        = 0.f;
                              float sampleCount = 0.f;
                              auto  blurFn      = [&](int x, int y)
                              {
                                if (constSampler.isInBounds(x, y))
                                {
                                  blur += constSampler.sampleUnsafe(x, y);
                                  sampleCount += 1.f;
                                }
                              };
                              for (int x = 0; x < sampler.width; ++x)
                              {
                                for (int py = -1; py <= 1; ++py)
                                {
                                  for (int px = -1; px <= 1; ++px)
                                  {
                                    blurFn(x + px, y + py);
                                  }
                                }
                                constexpr float ninex = 1.f / 9.f;
                                sampler.madd((blur * factor) / sampleCount, 1 - factor, x, y);
                              }
                            });
    }
  }
}

void PostProcess_hwy()
{
  auto builder = buildMeta<NodeMeta_hwy>("@PostProcess"_ls, "postprocess");
  builder.outputs(DataFormat(DataTypeEnum::ePostProcess));

  {
    builder.add<PostProcessNode>(NoDomain(), "@postProcess", IconOpPostProc);
    builder.param<&PostProcessNode::source>("@source");
    builder.fn(postProcessPassthrough);
    builder.done();
  }

  {
    builder.add<ErosionNode>(NoDomain(), "@hydraulicParticleErosion", IconOpErosion);
    builder.fn(postProcessPassthrough);
    builder.prepare(hydraulicErosion_prepare);
    builder.end(hydraulicErosion_end);
    builder.param<&ErosionNode::source>("@source", FmtVal<DataTypeEnum::ePostProcess>());
    builder.param<&ErosionNode::relativePos>("@relativePos", FmtVal<DataTypeEnum::eFloat2>(0.5f, 0.1f, 1.0f, 0.01f));
    builder.param<&ErosionNode::effectRadius>("@effectRadius");
    builder.param<&ErosionNode::particleCount>("@particleCount", FmtVal<DataTypeEnum::eInt>());
    builder.param<&ErosionNode::iteration>("@iteration",
                                           FmtVal<DataTypeEnum::eInt>(0, std::numeric_limits<int>::max()));
    builder.param<&ErosionNode::lifetime>("@lifetime");
    builder.param<&ErosionNode::inertia>("@inertia");
    builder.param<&ErosionNode::minCapacity>("@minCapacity");
    builder.param<&ErosionNode::maxCapacity>("@maxCapacity");
    builder.param<&ErosionNode::minSlope>("@minSlope");
    builder.param<&ErosionNode::depositRate>("@depositRate");
    builder.param<&ErosionNode::erosionRate>("@erosionRate");
    builder.param<&ErosionNode::erodeRadius>("@erosionRadius");
    builder.param<&ErosionNode::evaporationRate>("@evaporationRate");
    builder.param<&ErosionNode::gravity>("@gravity");
    builder.param<&ErosionNode::minSediment>("@minSediment");
    builder.param<&ErosionNode::erosionMask>("@erosionMask", FmtVal<DataTypeEnum::eImage>());
    builder.param<&ErosionNode::blur>("@applyBlur");
    builder.param<&ErosionNode::blurFactor>("@blurFactor");
    builder.done();
  }
}

} // namespace terra