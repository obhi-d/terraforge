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
  void tick() final;
  bool hasResults() final;
  bool getResults(GfxImage::handle& heights, GfxImage::handle& layerContrib) final;

  void describeBuffer(HybridBuffer::handle, HDataSource owner, uint32_t size, ImageFormatEnum format);
  void describeImage(HybridBuffer::handle, HDataSource owner, uint32_t width, uint32_t height, ImageFormatEnum format);

  GfxBuffer::handle        readBuffer(HybridBuffer::handle);
  GfxImage::handle         readImage(HybridBuffer::handle);
  std::span<ubyte_t const> readBufferData(HybridBuffer::handle);
  std::span<ubyte_t const> readImageData(HybridBuffer::handle);
  GfxBuffer::handle        writeBuffer(HybridBuffer::handle, bool discard);
  GfxImage::handle         writeImage(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeBufferData(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeImageData(HybridBuffer::handle, bool discard);

  void cleanup() final;
  void push(HDataSource);

private:
  void execute();

  using UseSet   = std::unordered_set<HybridBuffer::handle, HybridBuffer::hasher>;
  using OrderSet = std::unordered_set<HDataSource, HHashSource>;

  size_t                          memoryUsed_    = 0;
  size_t                          devMemoryUsed_ = 0;
  HybridNode::Result              result_        = HybridNode::Result::eWaiting;
  uint32_t                        tick_          = 1;
  uvec2                           tileId_;
  HDataSource                     actor_;
  HDataSource                     current_;
  UseSet                          recents_;
  OrderSet                        orderSet_;
  std::vector<Node>               ordered_;
  acl::sparse_table<HybridBuffer> buffers_;
};

} // namespace terra