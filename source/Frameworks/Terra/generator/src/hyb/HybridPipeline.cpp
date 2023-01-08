
#include "hyb/HybridPipeline.h"
#include "GpuMinMax.h"
#include "Terra.h"
#include "hyb/HybridNode.h"

namespace terra
{
/// ============ Debug ============
constexpr bool Debug = false;
/// ===============================

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

    memoryUsed_    = 0;
    devMemoryUsed_ = 0;
    result_        = HybridNode::Result::eWaiting;
    cversion_      = version_;
  }

  if (result_ == HybridNode::Result::eWaiting && actor_)
  {
    get().get<HybridNode>(actor_).prepare(*this);
    orderSet_.clear();
  }

  if (!actor_)
    return false;

  if (result_ == HybridNode::Result::eWaiting || result_ == HybridNode::Result::eContinue)
  {
    execute();
    auto buffer = getBuffer(Semantic::heights);
    if (buffer.buffer_)
    {
      auto image = buffers_[buffer.buffer_].image();
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
    iteration_ = ++node.iteration;
    result_    = HybridNode::Result::eContinue;
    break;
  case HybridNode::Result::eDone:
    if constexpr (Debug)
    {
      if (ordered_.size() > 1)
        ordered_.pop_back();
    }
    else
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
    auto h = *it;

    auto& ii = buffers_.at(h);
    if (ii.lastUsed() < tick_ - 2)
    {
      if (ii.offload())
      {
        auto s = ii.size();
        devMemoryUsed_ -= s;
      }
    }
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
  auto bheights    = getBuffer(Semantic::heights);
  auto bwater      = getBuffer(Semantic::water);
  auto brocks      = getBuffer(Semantic::rocks);
  auto bvegetation = getBuffer(Semantic::vegetation);

  heights                 = bheights.buffer_ ? readImage(bheights.buffer_) : GfxImage::handle{};
  layerContrib.water      = bwater.buffer_ ? readImage(bwater.buffer_) : GfxImage::handle{};
  layerContrib.rocks      = brocks.buffer_ ? readImage(brocks.buffer_) : GfxImage::handle{};
  layerContrib.vegetation = bvegetation.buffer_ ? readImage(bvegetation.buffer_) : GfxImage::handle{};
}

GfxSampler::handle HybridPipeline::getSampler(SamplerParamEnum sampler)
{
  if (!samplers_[sampler])
  {
    switch (sampler)
    {
    case SamplerParamEnum::eLinearWrap:
      samplers_[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eLinear));
      break;
    case SamplerParamEnum::eLinearClamp:
      samplers_[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
      break;
    case SamplerParamEnum::eTrilinearWrap:
      samplers_[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eTrilinear));
      break;
    case SamplerParamEnum::eNearestWrap:
      samplers_[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eNearest));
      break;
    case SamplerParamEnum::eNearestClamp:
      samplers_[sampler] = get().getDevice().createSampler(ImageSampling(SamplingType::eNearest, Tiling::eClampToEdge));
      break;
    }
  }
  return samplers_[sampler];
}

void HybridPipeline::cleanup()
{
  buffers_.clear();
  ordered_.clear();
  for (auto& sampler : samplers_)
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