
#include "hyb/HybridPipeline.h"
#include "Terra.h"
#include "hyb/HybridNode.h"

namespace terra
{

HybridPipeline::HybridPipeline()
{
  buffers.emplace();
}

HybridBuffer::handle HybridPipeline::declareBuffer()
{
  return buffers.emplace();
}

void HybridPipeline::describeBuffer(HybridBuffer::handle item, HDataSource owner, uint32_t size, ImageFormatEnum format)
{
  auto& ii = buffers.at(item);
  ii       = HybridBuffer(owner, size, 1, format, false);
}

void HybridPipeline::describeImage(HybridBuffer::handle item, HDataSource owner, uint32_t width, uint32_t height,
                                   ImageFormatEnum format)
{
  auto& ii = buffers.at(item);
  ii       = HybridBuffer(owner, width, height, format, true);
}

GfxBuffer::handle HybridPipeline::readBuffer(HybridBuffer::handle item)
{
  auto& ii = buffers.at(item);
  ii.upload();
  ii.read(runCount);
  recents.emplace(item);
  return ii.getBuffer();
}

GfxImage::handle HybridPipeline::readImage(HybridBuffer::handle item)
{
  auto& ii = buffers.at(item);
  ii.upload();
  ii.read(runCount);
  recents.emplace(item);
  return ii.getImage();
}

std::span<ubyte_t const> HybridPipeline::readBufferData(HybridBuffer::handle item)
{
  auto& ii = buffers.at(item);
  ii.read(runCount);
  recents.emplace(item);
  ii.offload();
  return ii.ensureHost();
}

std::span<ubyte_t const> HybridPipeline::readImageData(HybridBuffer::handle item)
{
  auto& ii = buffers.at(item);
  ii.read(runCount);
  recents.emplace(item);
  ii.offload();
  return ii.ensureHost();
}

GfxBuffer::handle HybridPipeline::writeBuffer(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers.at(item);
  ii.use(runCount);
  ii.ensureDev();
  auto s = ii.size();
  if (!ii.getBuffer())
  {
    memoryUsed += s;
    devMemoryUsed += s;
  }
  if (!discard)
    ii.upload();
  recents.emplace(item);
  return ii.getBuffer();
}

GfxImage::handle HybridPipeline::writeImage(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers.at(item);
  ii.use(runCount);
  ii.ensureDev();
  auto s = ii.size();
  if (!ii.getImage())
  {
    memoryUsed += s;
    devMemoryUsed += s;
  }
  if (!discard)
    ii.upload();
  recents.emplace(item);
  return ii.getImage();
}

std::span<ubyte_t> HybridPipeline::writeBufferData(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers.at(item);
  ii.use(runCount);
  auto s = ii.size();
  if (!ii.getData())
  {
    memoryUsed += s;
  }
  if (!discard)
    ii.offload();
  auto ret = ii.ensureHost();
  recents.emplace(item);
  return ret;
}

std::span<ubyte_t> HybridPipeline::writeImageData(HybridBuffer::handle item, bool discard)
{
  return writeBufferData(item, discard);
}

void HybridPipeline::compute(HDataSource item, uvec2 tileSize, uvec2 start, float freq, uint32_t seed)
{
  actor           = item;
  this->frequency = freq;
  this->seed      = seed;
  this->start     = start;
  this->size      = tileSize;
  iteration       = 0;
  actor           = item;
  memoryUsed      = 0;
  devMemoryUsed   = 0;
  buffers.clear();
  ordered.clear();

  get().get<HybridNode>(item).prepare(*this);
  orderSet.clear();
}

void HybridPipeline::execute()
{
  if (ordered.empty())
  {
    result = HybridNode::Result::eWaiting;
    return;
  }
  Node&       node = ordered.back();
  HDataSource item = node.item;
  switch (get().get<HybridNode>(item).execute(*this))
  {
  case HybridNode::Result::eContinue:
    node.iteration++;
    result = HybridNode::Result::eContinue;
    break;
  case HybridNode::Result::eDone:
    ordered.pop_back();
    result = (ordered.empty()) ? HybridNode::Result::eDone : HybridNode::Result::eContinue;
    break;
  case HybridNode::Result::eFailed:
    result = HybridNode::Result::eFailed;
    ordered.clear();
    break;
  }
  auto it = recents.begin();
  while (it != recents.end())
  {
    auto& ii = buffers.at(*it);
    if (ii.lastUsed() < runCount)
    {
      if (ii.offload())
      {
        auto s = ii.size();
        devMemoryUsed -= s;
      }

      it = recents.erase(it);
    }
    else if (ii.isDetached())
    {
      auto s = ii.size();
      memoryUsed -= s;
      if (ii.getBuffer())
        devMemoryUsed -= s;
      ii.clear();
    }
    else
      ++it;
  }
}

void HybridPipeline::push(HDataSource item)
{
  if (!orderSet.contains(item))
  {
    ordered.emplace_back();
    ordered.back().item = item;
    orderSet.emplace(item);
  }
}

} // namespace terra