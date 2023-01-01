
#include "hyb/HybridPipeline.h"
#include "GpuMinMax.h"
#include "Terra.h"
#include "hyb/HybridNode.h"

namespace terra
{

HybridPipeline::HybridPipeline() {}

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

HybridPipeline::BufferAndSize HybridPipeline::readBuffer(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.upload();
  ii.read(tick_);
  recents_.emplace(item);
  return BufferAndSize(ii.buffer(), (uint32_t)ii.size());
}

GfxImage::handle HybridPipeline::readImage(HybridBuffer::handle item)
{
  auto& ii = buffers_.at(item);
  ii.upload();
  ii.read(tick_);
  recents_.emplace(item);
  return ii.image();
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
  if (!ii.buffer())
  {
    memoryUsed_ += s;
    devMemoryUsed_ += s;
  }
  if (!discard)
    ii.upload();
  recents_.emplace(item);
  if (ii.owner() == actor())
    ii.read(tick_);
  return ii.buffer();
}

GfxImage::handle HybridPipeline::writeImage(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers_.at(item);
  ii.use(tick_);
  ii.ensureDev();
  auto s = ii.size();
  if (!ii.image())
  {
    memoryUsed_ += s;
    devMemoryUsed_ += s;
  }
  if (!discard)
    ii.upload();
  recents_.emplace(item);
  if (ii.owner() == actor())
    ii.read(tick_);
  return ii.image();
}

std::span<ubyte_t> HybridPipeline::writeBufferData(HybridBuffer::handle item, bool discard)
{
  auto& ii = buffers_.at(item);
  ii.use(tick_);
  auto s = ii.size();
  if (!ii.data())
  {
    memoryUsed_ += s;
  }
  if (!discard)
    ii.offload();
  auto ret = ii.ensureHost();
  recents_.emplace(item);
  if (ii.owner() == actor())
    ii.read(tick_);
  return ret;
}

std::span<ubyte_t> HybridPipeline::writeImageData(HybridBuffer::handle item, bool discard)
{
  return writeBufferData(item, discard);
}

void HybridPipeline::compute(uvec2 tile)
{
  tile_ = tile;
  version_++;
}

bool HybridPipeline::tick()
{
  bool actorOutdated = false;
  if (!DataSource::isValid(actor_))
    actor_ = {};

  if (actor_)
  {
    auto ver      = get().get<HybridNode>(actor_).getVersion();
    actorOutdated = actorVersion_ != ver;
    actorVersion_ = ver;
    orderSet_.clear();
  }

  if (version_ != cversion_ || actorOutdated)
  {
    buffers_.clear();
    ordered_.clear();
    iteration_ = 0;

    heights_    = declareBuffer();
    water_      = declareBuffer();
    rocks_      = declareBuffer();
    vegetation_ = declareBuffer();
    describeImage(heights_, {}, size().x, size().y, ImageFormatEnum::eFloat);
    describeImage(water_, {}, size().x, size().y, ImageFormatEnum::eFloat);
    describeImage(rocks_, {}, size().x, size().y, ImageFormatEnum::eFloat);
    describeImage(vegetation_, {}, size().x, size().y, ImageFormatEnum::eFloat);

    memoryUsed_    = 0;
    devMemoryUsed_ = 0;
    result_        = HybridNode::Result::eWaiting;
    cversion_      = version_;
  }

  if (iteration_ == 0 && actor_)
  {
    get().get<HybridNode>(actor_).prepare(*this);
    orderSet_.clear();
  }

  if (result_ == HybridNode::Result::eWaiting || result_ == HybridNode::Result::eContinue)
  {
    execute();
    if (heights_)
    {
      auto image = buffers_[heights_].image();
      if (image)
      {
        minMax_ = GpuMinMax::execute(image, size());
      }
    }
    return true;
  }
  return false;
}

void HybridPipeline::execute()
{
  /// ============ Debug ============
  constexpr bool Debug = true;
  /// ===============================

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
    if constexpr (Debug)
    {
      result_ = HybridNode::Result::eContinue;
    }
    else
    {
      ordered_.pop_back();
      result_ = (ordered_.empty()) ? HybridNode::Result::eDone : HybridNode::Result::eContinue;
    }
    break;
  case HybridNode::Result::eFailed:
    result_ = HybridNode::Result::eFailed;
    ordered_.clear();
    break;
  }
  auto it = recents_.begin();
  while (it != recents_.end())
  {
    auto h = *it;

    if (h == heights_ || h == rocks_ || h == vegetation_ || h == water_)
    {
      ++it;
      continue;
    }

    auto& ii = buffers_.at(h);
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
      if (ii.buffer())
        devMemoryUsed_ -= s;
      ii.clear();

      it = recents_.erase(it);
    }
    else
      ++it;
  }
  recents_.clear();
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

void HybridPipeline::getResults(GfxImage::handle& heights, Layers& layerContrib)
{
  heights                 = heights_ ? readImage(heights_) : GfxImage::handle{};
  layerContrib.water      = water_ ? readImage(water_) : GfxImage::handle{};
  layerContrib.rocks      = rocks_ ? readImage(rocks_) : GfxImage::handle{};
  layerContrib.vegetation = vegetation_ ? readImage(vegetation_) : GfxImage::handle{};
}

GfxSampler::handle HybridPipeline::getSampler(SamplerParamEnum sampler)
{
  if (!samplers[sampler])
  {
    switch (sampler)
    {
    case SamplerParamEnum::eLinearWrap:
      samplers[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eLinear));
      break;
    case SamplerParamEnum::eTrilinearWrap:
      samplers[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eTrilinear));
      break;
    case SamplerParamEnum::eNearestWrap:
      samplers[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eNearest));
      break;
    }
  }
  return samplers[sampler];
}

void HybridPipeline::cleanup()
{
  buffers_.clear();
  ordered_.clear();
  for (auto& sampler : samplers)
  {
    get().getDevice().destroy(sampler);
    sampler = {};
  }
  actor_   = {};
  current_ = {};
  recents_.clear();
  Pipeline::cleanup();
}

} // namespace terra