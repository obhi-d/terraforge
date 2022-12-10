#include "Pipeline.h"
#include "Terra.h"

namespace terra
{

void Pipeline::actor(HDataSource ds)
{
  if (actor_ != ds)
  {
    actor_ = ds;
    version_++;
  }
}

void Pipeline::seed(uint32_t val)
{
  if (seed_ != val)
  {
    seed_ = val;
    version_++;
  }
}

void Pipeline::frequency(float val)
{
  if (frequency_ != val)
  {
    frequency_ = val;
    version_++;
  }
}

void Pipeline::size(ivec2 val)
{
  if (size_ != val)
  {
    size_ = val;
    version_++;
  }
}

void Pipeline::offset(ivec2 val)
{
  if (offset_ != val)
  {
    offset_ = val;
    version_++;
  }
}

/*
void Pipeline::compute(HDataSource h, LaunchParams const& params, ivec2 start, ivec2 size)
{
assert(start[0] >= 1);
assert(start[1] >= 1);

wait();
cleanup();
resetIteration();

this->actor        = h;
this->launchParams = params;
this->iteration    = 0;
this->start        = start;
this->size         = size;

// how many tiles
// top corner is (1, 1)
uvec2 tileCount;
tileCount[0] = (size[0] + ((start[0] - 1) % params.tileSize[0])) / params.tileSize[0];
tileCount[1] = (size[1] + ((start[1] - 1) % params.tileSize[1])) / params.tileSize[1];

uvec2 tileStart;
tileStart[0] = ((start[0] - 1) / params.tileSize[0]);
tileStart[1] = ((start[1] - 1) / params.tileSize[1]);

for (uint32_t i = 0; i < tileCount[0]; ++i)
{
  for (uint32_t j = 0; j < tileCount[1]; ++j)
  {
    EnvParams envParams;
    envParams.tile[0]              = i + tileStart[0];
    envParams.tile[1]              = j + tileStart[1];
    envParams.tileSize             = params.tileSize;
    envParams.outputSize           = size;
    envParams.startxy[0]           = start[0] + i * params.tileSize[0];
    envParams.startxy[1]           = start[1] + j * params.tileSize[1];
    envParams.tileRegion.offset[0] = std::max(envParams.startxy[0], start[0]) - envParams.startxy[0];
    envParams.tileRegion.offset[1] = std::max(envParams.startxy[1], start[1]) - envParams.startxy[1];
    envParams.tileRegion.size[0]   = params.tileSize[0] - envParams.tileRegion.offset[0];
    envParams.tileRegion.size[1]   = params.tileSize[1] - envParams.tileRegion.offset[1];
    envParams.region.offset[0]     = envParams.startxy[0] - start[0];
    envParams.region.offset[1]     = envParams.startxy[1] - start[1];
    envParams.region.size          = envParams.tileRegion.size;
    if (envParams.region.size[0] > 0 && envParams.region.size[1] > 0)
    {
      // envParams.textureOffset[0] = envParams.tileOffset[0] - start[0];
      // envParams.textureOffset[1] = envParams.tileOffset[1] - start[1];
      envParams.frequency = params.frequency;
      envParams.seed      = params.seed;
      // envParams.bufferArraySize  = (envParams.size[0] + 2) * (envParams.size[1] + 2);
      pushTileTask(envParams);
    }
  }
}

launch();
}
*/
void Pipeline::cleanup() {}

} // namespace terra