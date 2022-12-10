
#include "hyb/HybridPipeline.h"
#include "Terra.h"
#include "hyb/HybridNode.h"

namespace terra
{

HybridPipeline::HybridPipeline()
{
  buffers_.emplace();
}

HybridBuffer::handle HybridPipeline::declareBuffer()
{
  return buffers_.emplace();
}

void HybridPipeline::describeBuffer(HybridBuffer::handle item, HDataSource owner, uint32_t size, ImageFormatEnum format)
{
  auto& ii = buffers_.at(item);
  ii       = HybridBuffer(owner, size, 1, format, false);
}

void HybridPipeline::describeImage(HybridBuffer::handle item, HDataSource owner, uint32_t width, uint32_t height,
                                   ImageFormatEnum format)
{
  auto& ii = buffers_.at(item);
  ii       = HybridBuffer(owner, width, height, format, true);
}

GfxBuffer::handle HybridPipeline::readBuffer(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.upload();
  ii.read(tick_);
  recents_.emplace(item);
  return ii.getBuffer();
}

GfxImage::handle HybridPipeline::readImage(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.upload();
  ii.read(tick_);
  recents_.emplace(item);
  return ii.getImage();
}

std::span<ubyte_t const> HybridPipeline::readBufferData(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.read(tick_);
  recents_.emplace(item);
  ii.offload();
  return ii.ensureHost();
}

std::span<ubyte_t const> HybridPipeline::readImageData(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.read(tick_);
  recents_.emplace(item);
  ii.offload();
  return ii.ensureHost();
}

GfxBuffer::handle HybridPipeline::writeBuffer(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers_.at(item);
  ii.use(tick_);
  ii.ensureDev();
  auto s = ii.size();
  if (!ii.getBuffer())
  {
    memoryUsed_ += s;
    devMemoryUsed_ += s;
  }
  if (!discard)
    ii.upload();
  recents_.emplace(item);
  return ii.getBuffer();
}

GfxImage::handle HybridPipeline::writeImage(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers_.at(item);
  ii.use(tick_);
  ii.ensureDev();
  auto s = ii.size();
  if (!ii.getImage())
  {
    memoryUsed_ += s;
    devMemoryUsed_ += s;
  }
  if (!discard)
    ii.upload();
  recents_.emplace(item);
  return ii.getImage();
}

std::span<ubyte_t> HybridPipeline::writeBufferData(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers_.at(item);
  ii.use(tick_);
  auto s = ii.size();
  if (!ii.getData())
  {
    memoryUsed_ += s;
  }
  if (!discard)
    ii.offload();
  auto ret = ii.ensureHost();
  recents_.emplace(item);
  return ret;
}

std::span<ubyte_t> HybridPipeline::writeImageData(HybridBuffer::handle item, bool discard)
{
  return writeBufferData(item, discard);
}

void HybridPipeline::compute(uvec2 tile)
{
  tileId_ = tile;
  version_++;
}

void HybridPipeline::tick()
{
  if (version_ != cversion_)
  {
    iteration_ = 0;
    buffers_.clear();
    ordered_.clear();
    if (actor_)
    {
      get().get<HybridNode>(actor_).prepare(*this);
      orderSet_.clear();
    }
    memoryUsed_    = 0;
    devMemoryUsed_ = 0;
    result_        = HybridNode::Result::eWaiting;
  }
  if (result_ == HybridNode::Result::eWaiting || result_ == HybridNode::Result::eContinue)
  {
    execute();
  }
}

void HybridPipeline::execute()
{
  if (ordered_.empty())
  {
    result_ = HybridNode::Result::eWaiting;
    return;
  }
  Node&       node = ordered_.back();
  HDataSource item = node.item;
  switch (get().get<HybridNode>(item).execute(*this))
  {
  case HybridNode::Result::eContinue:
    node.iteration++;
    result_ = HybridNode::Result::eContinue;
    break;
  case HybridNode::Result::eDone:
    ordered_.pop_back();
    result_ = (ordered_.empty()) ? HybridNode::Result::eDone : HybridNode::Result::eContinue;
    break;
  case HybridNode::Result::eFailed:
    result_ = HybridNode::Result::eFailed;
    ordered_.clear();
    break;
  }
  auto it = recents_.begin();
  while (it != recents_.end())
  {
    auto& ii = buffers_.at(*it);
    if (ii.lastUsed() < tick_)
    {
      if (ii.offload())
      {
        auto s = ii.size();
        devMemoryUsed_ -= s;
      }

      it = recents_.erase(it);
    }
    else if (ii.isDetached())
    {
      auto s = ii.size();
      memoryUsed_ -= s;
      if (ii.getBuffer())
        devMemoryUsed_ -= s;
      ii.clear();
    }
    else
      ++it;
  }
}

void HybridPipeline::push(HDataSource item)
{
  if (!orderSet_.contains(item))
  {
    ordered_.emplace_back();
    ordered_.back().item = item;
    orderSet_.emplace(item);
  }
}

bool HybridPipeline::hasResults()
{
  return (result_ == HybridNode::Result::eContinue || result_ == HybridNode::Result::eDone);
}

bool HybridPipeline::getResults(GfxImage::handle& heights, GfxImage::handle& layerContrib) {}

} // namespace terra