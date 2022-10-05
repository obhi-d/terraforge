
#pragma once

#include "Common.h"
#include "GpuBuffer.h"
#include "Node.h"
#include <variant>

namespace terra
{

class Terra;
struct DispatchTask
{
  EnvParams params;
  uint32_t  taskID = 0;
};
class Pipeline
{
public:

  ~Pipeline()
  {
    cleanup();
  }

  void    prepare(DispatchTask&&);
  int32_t compute(hnode);
  int32_t computePreview(hnode); // wrap in Modifiers.toRGBA

  int32_t declBuffer(uint32_t size);
  int32_t declImage(uint32_t width, uint32_t height, ImageFormat fmt);

  // Read for the given node list index
  GfxBuffer::handle            readOutputBuffer(int32_t nodeListIdx) const;
  GfxImage2D::handle           readOutputImage(int32_t nodeListIdx) const;
  void                         readOutputBufferContent(int32_t nodeIdx, std::span<std::byte>) const;
  void                         readOutputImageContent(int32_t nodeIdx, std::span<std::byte>) const;

  // Read for the given node
  GfxBuffer::handle  getOutputBuffer(hnode nodeIdx) const;
  GfxImage2D::handle getOutputImage(hnode nodeIdx) const;

  void execute();
  void wait();
  void cleanup();

  EnvParams const& params() const
  {
    return task.params;
  }

  GpuBuffer& getUbo()
  {
    return ubo;
  }

private:
  void determineBufferLifetimes();
  void gatherLeafs(hnode nodeid, std::unordered_map<int32_t, int>& edgeMap, std::vector<hnode>& leafs);
  void allocateResources();
  void sortNodes();

  void prepareNodes(hnode);

  // Images are not shared
  struct Image
  {
    GfxImage2D::handle image;
    ImageFormat        format = ImageFormat::eFloat;
    uint32_t           width  = 0;
    uint32_t           height = 0;

    ~Image();
  };

  // Buffers are reused
  struct Buffer
  {
    uint32_t size  = 0;
    int32_t  phyId = 0;
    int32_t  write = -1;
    int32_t  read  = -1;
  };

  std::vector<Image>  images;
  std::vector<Buffer> buffers;

  std::vector<GpuBuffer> gpuBuffers;

  std::vector<hnode> nodesToDelete;
  std::vector<hnode> nodes;
  // all nodes that are executed, and in order
  std::vector<hnode> actors;

  GpuBuffer        ubo;
  DispatchTask     task;
  GfxFence::handle sync;
};

} // namespace terra