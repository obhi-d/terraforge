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

  void describeBuffer(HybridBuffer::handle, HDataSource owner, uint32_t size, ImageFormatEnum format);
  void describeImage(HybridBuffer::handle, HDataSource owner, uint32_t width, uint32_t height, ImageFormatEnum format);

  GfxBuffer::handle        readBuffer(HybridBuffer::handle);
  GfxImage2D::handle       readImage(HybridBuffer::handle);
  std::span<ubyte_t const> readBufferData(HybridBuffer::handle);
  std::span<ubyte_t const> readImageData(HybridBuffer::handle);
  GfxBuffer::handle        writeBuffer(HybridBuffer::handle, bool discard);
  GfxImage2D::handle       writeImage(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeBufferData(HybridBuffer::handle, bool discard);
  std::span<ubyte_t>       writeImageData(HybridBuffer::handle, bool discard);

  void compute(HDataSource, uvec2 tileSize, uvec2 start, float freq, uint32_t seed);
  void push(HDataSource);

  uvec2 getStart() const
  {
    return start;
  }

  uvec2 getSize() const
  {
    return start;
  }

  uvec2 getTileId() const
  {
    return tileId;
  }

private:
  void execute();

  using UseSet                                  = std::unordered_set<HybridBuffer::handle, HybridBuffer::hasher>;
  using OrderSet                                = std::unordered_set<HDataSource, HHashSource>;
  size_t                          memoryUsed    = 0;
  size_t                          devMemoryUsed = 0;
  HybridNode::Result              result        = HybridNode::Result::eWaiting;
  uint32_t                        runCount      = 1;
  float                           frequency     = 0.01f;
  uint32_t                        seed          = 0;
  uvec2                           start;
  uvec2                           size;
  uvec2                           tileId;
  HDataSource                     actor;
  HDataSource                     current;
  UseSet                          recents;
  OrderSet                        orderSet;
  std::vector<Node>               ordered;
  acl::sparse_table<HybridBuffer> buffers;
};

} // namespace terra