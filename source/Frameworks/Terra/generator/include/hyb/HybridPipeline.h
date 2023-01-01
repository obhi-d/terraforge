#pragma once

#include "GpuBuffer.h"
#include "HashMachine.h"
#include "Pipeline.h"
#include "Table.h"
#include "hyb/HybridBuffer.h"
#include "hyb/HybridNode.h"
#include <acl/sparse_table.hpp>
#include <span>
#include <unordered_set>

namespace acl
{
template <>
struct traits<terra::HybridBuffer>
{
  using size_type                              = std::uint32_t;
  static constexpr std::uint32_t pool_size     = 32;
  static constexpr std::uint32_t idx_pool_size = 32;
  static constexpr bool          assume_pod_v  = false;
};
} // namespace acl

namespace terra
{

class HybridPipeline : public Pipeline
{

  struct Node
  {
    HDataSource item;
    uint32_t    iteration = 0;
  };

public:
  HybridPipeline();

  HybridBuffer::handle declareBuffer();

  void compute(uvec2 tile) final;
  bool tick() final;
  void getResults(GfxImage::handle& heights, Layers& layerContrib) final;

  void describeBuffer(HybridBuffer::handle, HDataSource owner, uint32_t size, ImageFormatEnum format);
  void describeImage(HybridBuffer::handle, HDataSource owner, uint32_t width, uint32_t height, ImageFormatEnum format);

  using BufferAndSize = std::pair<GfxBuffer::handle, uint32_t>;
  GfxSampler::handle       getSampler(SamplerParamEnum sampler);
  BufferAndSize            readBuffer(HybridBuffer::handle);
  GfxImage::handle         readImage(HybridBuffer::handle);
  std::span<ubyte_t const> readBufferData(HybridBuffer::handle);
  std::span<ubyte_t const> readImageData(HybridBuffer::handle);
  GfxBuffer::handle        writeBuffer(HybridBuffer::handle, bool discard);
  GfxImage::handle         writeImage(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeBufferData(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeImageData(HybridBuffer::handle, bool discard);

  void cleanup() final;
  void push(HDataSource);

  HybridBuffer::handle heights() const
  {
    return heights_;
  }

  HybridBuffer::handle water() const
  {
    return water_;
  }

  HybridBuffer::handle rocks() const
  {
    return rocks_;
  }

  HybridBuffer::handle terrain() const
  {
    return rocks_;
  }

  HybridBuffer::handle vegetation() const
  {
    return vegetation_;
  }

private:
  void execute();

  using UseSet   = std::unordered_set<HybridBuffer::handle, HybridBuffer::hasher>;
  using OrderSet = std::unordered_set<HDataSource, HHashSource>;

  std::array<GfxSampler::handle, SamplerParam::kCount> samplers;

  HybridBuffer::handle heights_;
  HybridBuffer::handle water_;
  HybridBuffer::handle rocks_;
  HybridBuffer::handle vegetation_;

  size_t                          memoryUsed_    = 0;
  size_t                          devMemoryUsed_ = 0;
  HybridNode::Result              result_        = HybridNode::Result::eWaiting;
  uint32_t                        tick_          = 1;
  uint32_t                        actorVersion_  = 0xffffffff;
  HDataSource                     current_;
  UseSet                          recents_;
  OrderSet                        orderSet_;
  std::vector<Node>               ordered_;
  acl::sparse_table<HybridBuffer> buffers_;
};

} // namespace terra