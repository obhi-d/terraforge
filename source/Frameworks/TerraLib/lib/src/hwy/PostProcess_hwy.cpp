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
      // TODO spawn particles on the tile, and init position within
      // effect radius
      auto&    tile   = pipe.getTileData(i);
      auto&    ed     = data.data[i];
      uint32_t minX   = (uint32_t)std::floor(tile.params.tileSize[0] * node.relativePos[0]);
      uint32_t minY   = (uint32_t)std::floor(tile.params.tileSize[1] * node.relativePos[1]);
      uint32_t dx     = (uint32_t)std::floor(std::min(tile.params.tileSize[0] - minX, minX) * (.1 + node.effectRadius));
      uint32_t dy     = (uint32_t)std::floor(std::min(tile.params.tileSize[1] - minY, minY) * (.1 + node.effectRadius));
      uint32_t nbMin  = (uint32_t)std::min(node.minParticles, node.maxParticles);
      uint32_t nbCnt  = (uint32_t)std::max(node.minParticles, node.maxParticles) - nbMin + 1;
      uint32_t nbPart = nbMin + (wyrand(&data.seed) % nbCnt);
      // DEBUG TODO
      nbPart = 1;
      ed.particles.resize(nbPart);
      minX -= dx;
      minY -= dy;
      ed.min = ivec2{std::max(0, (int)minX), std::max(0, (int)minY)};
      dx <<= 1;
      dy <<= 1;
      ed.max =
        ivec2{std::min(tile.params.tileSize[0], (int)(minX + dx)), std::min(tile.params.tileSize[0], (int)(minY + dy))};
      dx = ed.max[0] - ed.min[0];
      dy = ed.max[1] - ed.min[1];
      // select kernel

      get().pool().for_each((uint32_t)0, nbPart, 16,
                            [&, fx = (float)dx, fy = (float)dy](uint32_t p)
                            {
                              auto&    particle = ed.particles[p];
                              uint64_t seed     = data.seed + p * node.randomizer;
                              particle.pos[0] = (float)(ed.min[0]) + (((uint32_t)wyrand(&seed) % 10000) / 10000.f) * fx;
                              particle.pos[1] = (float)(ed.min[1]) + (((uint32_t)wyrand(&seed) % 10000) / 10000.f) * fy;
                              particle.inertia = node.baseInertia +
                                                 (node.inertiaJitter * (float)((uint32_t)wyrand(&seed) % 255) / 255.f);
                              particle.velocity = 0.f;
                              particle.water    = node.dropletVolume;
                            });
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

  constexpr std::array<ivec2, 8> indices = {{
    {-1, -1},
    {0, -1},
    {1, -1},
    {-1, 0},
    {1, 0},
    {-1, 1},
    {0, 1},
    {1, 1},
  }};

  for (uint32_t tileIdx = 0; tileIdx < numTiles; ++tileIdx)
  {
    auto& tile = pipe.getTileData(tileIdx);
    auto& ed   = data.data[tileIdx];

    auto sampler = Sampler2D(tile.buffer.data(), tile.params.tileSize[0], tile.params.tileSize[1]);
    auto respawn = [&](Particle& particle, uint32_t p)
    {
      uint64_t seed     = data.seed + p * node.randomizer;
      particle.pos.x    = (float)(ed.min[0]) + (((uint32_t)wyrand(&seed) % 10000) / 10000.f) * (ed.max[0] - ed.min[0]);
      particle.pos.x    = (float)(ed.min[1]) + (((uint32_t)wyrand(&seed) % 10000) / 10000.f) * (ed.max[1] - ed.min[1]);
      particle.inertia  = node.baseInertia + (node.inertiaJitter * (float)((uint32_t)wyrand(&seed) % 255) / 255.f);
      particle.velocity = 0.f;
      particle.water    = node.dropletVolume;
      particle.life     = node.lifetime;
      particle.deposit  = 0.f;
    };

    get().pool().for_each(ed.particles.begin(), ed.particles.end(), 8,
                          [&](Particle& p)
                          {
                            uint32_t ix = (uint32_t)p.pos.x;
                            uint32_t iy = (uint32_t)p.pos.y;
                            if (!ed.isInBounds(ix, iy) || --p.life <= 0)
                            {
                              respawn(p, ix * iy);
                              return;
                            }
                            auto g      = sampler.heightGradientAt(p.pos);
                            auto ndir   = sub(scale(p.inertia, p.dir), scale(1.f - p.inertia, g));
                            auto length = std::sqrt(ndir.x * ndir.x + ndir.y * ndir.y);
                            if (length == 0.f)
                            {
                              respawn(p, ix * iy);
                              return;
                            }
                            length    = 1 / length;
                            p.dir     = length * ndir;
                            p.prevPos = p.pos;
                            p.pos += p.dir;
                            ix = (uint32_t)p.pos.x;
                            iy = (uint32_t)p.pos.y;
                            if (!sampler.isInBounds(ix, iy))
                            {
                              respawn(p, ix * iy);
                              return;
                            }
                            float hold  = sampler.bisample(p.prevPos.x, p.prevPos.y);
                            float hnew  = sampler.bisample(p.pos.x, p.pos.y);
                            float hdiff = hnew - hold;
                            if (hdiff > 0.f)
                            {
                              float maxDrop = std::min(hdiff, p.sediment);
                              assert(maxDrop >= 0.0f);
                              p.deposit = maxDrop;
                              assert(p.sediment - p.deposit >= 0.0f);
                              p.sediment -= p.deposit;
                            }
                            else
                            {
                              float capacity =
                                std::max(-hdiff, node.minSlope) * p.velocity * p.water * node.maxCapacity;
                              if (p.sediment > capacity)
                              {
                                float maxDrop = (p.sediment - capacity) * node.depositRate;
                                assert(maxDrop >= 0.0f);
                                p.deposit = maxDrop;
                                assert(p.sediment - p.deposit >= 0.0f);
                                p.sediment -= p.deposit;
                              }
                              else
                              {
                                p.deposit = std::min((capacity - p.sediment) * node.erosionRate, -hdiff);
                                assert(p.deposit >= 0.0f);
                                if (data.erosionMask)
                                {
                                  auto const& image = get().get<Image>(node.erosionMask.source);
                                  p.deposit *=
                                    image.sample(p.pos.x / tile.params.tileSize.x, p.pos.y / tile.params.tileSize.y);
                                }
                                assert(p.sediment + p.deposit >= 0.0f);
                                p.sediment += p.deposit;
                                p.deposit = -p.deposit;
                              }
                            }

                            p.velocity = std::sqrt(std::max(0.f, p.velocity * p.velocity + hdiff * node.gravity));
                            p.water *= (1.f - node.evaporationRate);
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
              std::max(0.f, node.erodeRadius - std::sqrt(distanceSq(p.pos, vec2{(float)sx, (float)sy})));
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
        int   px = (int)std::floor(p.pos.x);
        int   py = (int)std::floor(p.pos.y);
        float u  = (p.pos.x - std::floor(p.pos.x));
        float v  = (p.pos.y - std::floor(p.pos.y));

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
  builder.outputs(DataFormat(DataType::ePostProcess));

  {
    builder.add<PostProcessNode>(NoDomain(), "@postProcess", IconOpPostProc);
    builder.param<&PostProcessNode::source>("@source");
    builder.fn(postProcessPassthrough);
    builder.done();
  }

  {
    builder.add<ErosionNode>(NoDomain(), "@erosion", IconOpErosion);
    builder.fn(postProcessPassthrough);
    builder.prepare(hydraulicErosion_prepare);
    builder.end(hydraulicErosion_end);
    builder.param<&ErosionNode::source>("@source", FmtVal<DataType::ePostProcess>());
    builder.param<&ErosionNode::relativePos>("@relativePos", FmtVal<DataType::eFloat2>(0.5f, 0.1f, 1.0f, 0.01f));
    builder.param<&ErosionNode::effectRadius>("@effectRadius");
    builder.param<&ErosionNode::minParticles>("@minParticles", FmtVal<DataType::eInt>(5, 4, 100000, 1));
    builder.param<&ErosionNode::maxParticles>("@maxParticles", FmtVal<DataType::eInt>(5, 4, 100000, 1));
    builder.param<&ErosionNode::iteration>("@iteration", FmtVal<DataType::eInt>(0, std::numeric_limits<int>::max()));
    builder.param<&ErosionNode::lifetime>("@lifetime");
    builder.param<&ErosionNode::baseInertia>("@baseInertia");
    builder.param<&ErosionNode::inertiaJitter>("@inertiaJitter");
    builder.param<&ErosionNode::maxCapacity>("@maxCapacity");
    builder.param<&ErosionNode::dropletVolume>("@dropletVolume");
    builder.param<&ErosionNode::minSlope>("@minSlope");
    builder.param<&ErosionNode::depositRate>("@depositRate");
    builder.param<&ErosionNode::erosionRate>("@erosionRate");
    builder.param<&ErosionNode::erodeRadius>("@erosionRadius");
    builder.param<&ErosionNode::evaporationRate>("@evaporationRate");
    builder.param<&ErosionNode::gravity>("@gravity");
    builder.param<&ErosionNode::minSediment>("@minSediment");
    builder.param<&ErosionNode::randomizer>("@randomizer");
    builder.param<&ErosionNode::erosionMask>("@erosionMask", FmtVal<DataType::eImage>());
    builder.param<&ErosionNode::blur>("@applyBlur");
    builder.param<&ErosionNode::blurFactor>("@blurFactor");
    builder.done();
  }
}

} // namespace terra