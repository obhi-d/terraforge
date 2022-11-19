#include "Icons.h"
#include "Node.h"
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
  if (numTiles > 0)
  {
    data.data = acl::dynamic_array<ErosionTileData>(numTiles);
    for (uint32_t i = 0; i < numTiles; ++i)
    {
      auto&    tile   = pipe.getTileData(i);
      auto&    ed     = data.data[i];
      uint32_t minX   = (uint32_t)std::floor(tile.params.tileSize[0] * node.relativePos[0]);
      uint32_t minY   = (uint32_t)std::floor(tile.params.tileSize[1] * node.relativePos[1]);
      uint32_t dx     = (uint32_t)std::floor(std::min(tile.params.tileSize[0] - minX, minX) * (.1 + node.effectRadius));
      uint32_t dy     = (uint32_t)std::floor(std::min(tile.params.tileSize[1] - minY, minY) * (.1 + node.effectRadius));
      uint32_t nbPart = (uint32_t)node.minParticles + (uint32_t)(wyrand(&data.seed) % (uint32_t)(dx * dy));
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
      for (uint32_t i = 0; i < nbPart; ++i)
      {
        ed.particles[i].pos[0] = ed.min[0] + ((uint32_t)wyrand(&data.seed) % dx);
        ed.particles[i].pos[1] = ed.min[1] + ((uint32_t)wyrand(&data.seed) % dy);
      }
    }
  }
}

void hydraulicErosion_end(Node& inode, Pipeline_hwy& pipe)
{
  auto& node = (ErosionNode&)inode;
  // only work on it if we can reissue the node
  auto& data = pipe.getCacheData<ErosionCacheData>(node.getSelf());
  if (pipe.getIteration() - data.iteration < node.iteration)
  {
    if (!pipe.reissue(inode.getSelf()))
      return;
  }
  else
    return;
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

  constexpr uint32_t first = 0;
  get().pool().for_each(
    first, numTiles,
    [&](uint32_t i)
    {
      auto& tile = pipe.getTileData(i);
      auto& ed   = data.data[i];

      get().pool().for_each(
        ed.particles.begin(), ed.particles.end(), 8,
        [&](auto& p)
        {
          std::array<float, 8> v = {};

          float x    = tile.sample((uint32_t)p.pos[0], (uint32_t)p.pos[1]);
          int   next = 0;
          float max  = tile.sample((uint32_t)(p.pos[0] + indices[0][0]), (uint32_t)(p.pos[1] + indices[0][1])) - x;
          float area = std::abs(max);
          for (int j = 1; j < 8; ++j)
          {
            float v = tile.sample((uint32_t)(p.pos[0] + indices[j][0]), (uint32_t)(p.pos[1] + indices[j][1])) - x;
            area += v;
            if (v > max)
            {
              next = j;
              max  = v;
            }
          }

          area *= .25f;

          ivec2 nextp = {
            p.pos[0] + indices[next][0],
            p.pos[1] + indices[next][1],
          };
          if ((p.volume < node.minVolume) || (!ed.isInBounds(nextp[0], nextp[1])))
            p.dead = true;
          else
          {
            float sediment = p.volume * max;
            if (sediment < 0.0)
              sediment = 0.0;
            float sdiff = sediment - p.sediment;

            // Act on the Heightmap and Droplet!
            p.sediment += node.depositRate * sdiff;
            p.deposit = p.volume * node.depositRate * sdiff;
            p.volume *= area * (1.0f - node.evaporationRate);
          }
        });

      auto piend = (uint32_t)ed.particles.size();
      for (uint32_t pi = 0; pi < piend; ++pi)
      {
        auto& p = ed.particles[pi];
        if (p.dead)
        {
          std::swap(p, ed.particles.back());
          piend--;
        }

        if (!piend)
          break;

        tile.sample(p.pos[0], p.pos[1]) -= p.deposit;
      }
      ed.particles.resize(piend);
    });
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
    builder.param<&ErosionNode::iteration>("@iteration", FmtVal<DataType::eInt>(0, std::numeric_limits<int>::max()));
    builder.param<&ErosionNode::relativePos>("@relativePos", FmtVal<DataType::eFloat2>(0.5f, 0.1f, 0.2f, 0.8f));
    builder.param<&ErosionNode::effectRadius>("@effectRadius");
    builder.param<&ErosionNode::density>("@density");
    builder.param<&ErosionNode::evaporationRate>("@evaporationRate");
    builder.param<&ErosionNode::depositRate>("@depositRate");
    builder.param<&ErosionNode::minVolume>("@minVolume");
    builder.param<&ErosionNode::friction>("@friction");
    builder.param<&ErosionNode::minParticles>("@minParticles", FmtVal<DataType::eInt>(5, 4, 100000, 1));
    builder.done();
  }
}

} // namespace terra