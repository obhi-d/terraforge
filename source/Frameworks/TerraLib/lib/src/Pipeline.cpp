#include "Pipeline.h"
#include "Terra.h"

namespace terra
{

Pipeline::Image::~Image() 
{
  terra::get().getDevice().destroy(image);
}

Pipeline::Pipeline(uint32_t taskId) : id(taskId) 
{
  ubo.setDesc(GfxBuffer::Usage::fUniform, GfxStorageClass::eDynamicDeviceReadonly);
}

void Pipeline::compute(dshandle h, LaunchParams const& params, uvec2 start, uvec2 size)
{
  assert(start[0] >= 1);
  assert(start[1] >= 1);

  cleanup();

  actor      = h;
  launchParams = params;
  iteration  = 0;
  subTask      = 0;

  // how many tiles
  // top corner is (1, 1)
  uvec2 tileCount;
  tileCount[0] = (size[0] + ((start[0] - 1) % params.tileSize[0])) / params.tileSize[0]; 
  tileCount[1] = (size[1] + ((start[1] - 1) % params.tileSize[1])) / params.tileSize[1]; 

  uvec2 tileStart;
  tileStart[0] = ((start[0] - 1) / params.tileSize[0]);
  tileStart[1] = ((start[1] - 1) / params.tileSize[1]);

  tiles.reserve(tileCount[0] * tileCount[1]);

  for (uint32_t i = 0; i < tileCount[0]; ++i)
  {
    for (uint32_t j = 0; j < tileCount[1]; ++j)
    {
      TileTask task;
      task.envParams.tile[0] = i + tileStart[0];
      task.envParams.tile[1] = i + tileStart[1];
      
      task.envParams.tileOffset[0] = tileStart[0] + i * params.tileSize[0];
      task.envParams.tileOffset[1] = tileStart[1] + i * params.tileSize[1];
      task.envParams.tileSize      = params.tileSize;
      task.envParams.offset[0]     = std::max(task.envParams.tileOffset[0], start[0]) - task.envParams.tileOffset[0];
      task.envParams.offset[1]     = std::max(task.envParams.tileOffset[1], start[1]) - task.envParams.tileOffset[1];
      task.envParams.size[0]       = params.tileSize[0] - task.envParams.offset[0];
      task.envParams.size[1]       = params.tileSize[1] - task.envParams.offset[1];
      if (task.envParams.size[0] > 0 && task.envParams.size[1] > 0)
      {
        task.envParams.textureOffset[0] = task.envParams.tileOffset[0] - start[0];
        task.envParams.textureOffset[1] = task.envParams.tileOffset[1] - start[1];
        task.envParams.frequency        = params.frequency;
        task.envParams.wavelength       = params.wavelength;
        task.envParams.seed             = params.seed;
        task.envParams.bufferArraySize  = (task.envParams.size[0] + 2) * (task.envParams.size[1] + 2);
        tiles.emplace_back(task);
      }
    }
  }
  dirty = true;
  
}

void Pipeline::run() 
{
  if (dirty)
    return;

  dirty = false;
  subTask = 0;
  for (auto& t : tiles)
  {
    
    if (DataSource::isValid(actor))
    {
      get().get<Node>(actor).ensure(*this);
    }

    allocateResources();

    for (auto h : tiles[subTask].tasks)
    {
      auto result = get().get<Node>(h).run(*this);
      if (Result::eAbort == result)
        return;
      if (Result::eContinue == result)
        dirty = true;
    }

    subTask++;
  }
}

int32_t Pipeline::declBuffer(int32_t declIdx, uint32_t size, bool transient)
{
  auto& buffers = tiles[subTask].buffers;
  if (declIdx < 0 || declIdx >= (int)buffers.size())
  {
    declIdx = (int)buffers.size();
    buffers.emplace_back();
  }
  if (buffers[declIdx].size < size)
  {
    buffers[declIdx].size = size;
    buffers[declIdx].phyId = 0;
    buffers[declIdx].transient = transient;
  }
  return declIdx;
}

int32_t Pipeline::declImage(int32_t declIdx, uint32_t width, uint32_t height, ImageFormat fmt, bool transient) 
{
  auto& images = tiles[subTask].images;
  if (declIdx < 0 || declIdx >= (int)images.size())
  {
    declIdx = (int)images.size();
    images.emplace_back();
  }
  if (images[declIdx].width < width || images[declIdx].height < height || images[declIdx].format != fmt)
  {
    images[declIdx].width     = width;
    images[declIdx].height    = height;
    images[declIdx].format    = fmt;
    if (images[declIdx].image)
      get().getDevice().destroy(images[declIdx].image);
    images[declIdx].image     = {};
    images[declIdx].transient = transient;
  }
  return declIdx;
}

void Pipeline::determineBufferLifetimes()
{
  auto& main = Terra::get();
  auto& actors = tiles[subTask].tasks;
  auto& buffers = tiles[subTask].buffers;
  const auto taskid  = taskId();
  for (int32_t i = 0; i < (int32_t)actors.size(); ++i)
  {
    auto& nodeId = actors[i];
    auto& node   = main.get<Node>(nodeId);
    if (!node.hasTextureOutput())
    {
      auto id = node.getOutputId(taskid);
      if (id >= 0)
      {
        if (buffers[id].transient)
          buffers[id].write = i;
        else
        {
          // persistent buffer lives full range, to next frame
          buffers[id].write = 0;
          buffers[id].read  = (int32_t)actors.size();
        }
      }
    }
    node.forEachSource(
      [&](uint32_t p, dshandle innode) -> bool
      {
        auto& src   = main.get<Node>(innode);
        auto  srcid = src.getOutputId(taskid);
        if (srcid >= 0)
        {
          buffers[srcid].read = std::max(buffers[srcid].read, i);
        }
        return true;
      });
  }
}

void Pipeline::allocateResources()
{
  determineBufferLifetimes();

  auto&    buffers = tiles[subTask].buffers;
  uint32_t phyId  = 0;
  uint32_t nbBuff = (uint32_t)buffers.size();
  for (uint32_t i = 0; i < nbBuff; ++i)
  {
    assert(buffers[i].write < buffers[i].read);
    if (buffers[i].phyId)
      continue;
    buffers[i].phyId = ++phyId;
    
    for (uint32_t j = i + 1; j < nbBuff; ++j)
    {
      // x1 <= y2 && y1 <= x2
      if (buffers[j].phyId || !buffers[j].transient || !(buffers[i].read < buffers[j].write || buffers[j].read < buffers[i].write))
        continue;
      buffers[j].phyId = phyId;
    }
  }
  auto& gpuBuffers = tiles[subTask].gpuBuffers;
  gpuBuffers.resize(phyId);
  for (uint32_t i = 0; i < nbBuff; ++i)
    gpuBuffers[buffers[i].phyId].setSize(buffers[i].size);

  for (auto& gpu : gpuBuffers)
  {
    gpu.setDesc(GfxBuffer::Usage::fStorage, GfxStorageClass::eDynamicDeviceAccess);
    gpu.ensure();
  }

  auto& rd = Terra::get().getDevice();
  auto& images = tiles[subTask].images;
  for (auto& img : images)
  {
    if (!img.image)
      img.image = rd.createImage(GfxStorageClass::eDeviceAccess, img.width, img.height, img.format);
  }   
}

void Pipeline::cleanup() 
{
  auto& rd = Terra::get().getDevice();
  for (auto& t : tiles)
  {
    for (auto& img : t.images)
      rd.destroy(img.image);
  }
  tiles.clear();
}

GfxBuffer::handle  Pipeline::getOutputBuffer(dshandle nodeIdx) const 
{
  auto const& node = terra::get().get<Node>(nodeIdx);
  auto& buffers = tiles[subTask].buffers;
  return tiles[subTask].gpuBuffers[(uint32_t)buffers[(uint32_t)node.getOutputId(taskId())].phyId].get();
}

GfxImage2D::handle Pipeline::getOutputImage(dshandle nodeIdx) const 
{
  auto const& node = terra::get().get<Node>(nodeIdx);
  auto&       images = tiles[subTask].images;
  return images[(uint32_t)node.getOutputId(taskId())].image;
}

void Pipeline::enqueue(dshandle task) 
{
  tiles[subTask].tasks.emplace_back(task);
}

} // namespace terra