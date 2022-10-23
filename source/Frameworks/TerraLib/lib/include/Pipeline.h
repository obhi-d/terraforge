
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

  uint32_t taskId = 0;
};
class Pipeline
{
public:
  Pipeline(uint32_t taskId);
  ~Pipeline()
  {
    cleanup();
  }

  void compute(dshandle, LaunchParams const&, uvec2 start, uvec2 size); // wrap in Modifiers.toR16
  void run();

  int32_t declBuffer(int32_t declIdx, uint32_t size, bool transient);
  int32_t declImage(int32_t declIdx, uint32_t width, uint32_t height, ImageFormat fmt, bool transient);

  // Read for the given node
  GfxBuffer::handle  getOutputBuffer(dshandle nodeIdx) const;
  GfxImage2D::handle getOutputImage(dshandle nodeIdx) const;

  void cleanup();
  void enqueue(dshandle);

  EnvParams const& params() const
  {
    return tiles[subTask].envParams;
  }

  GpuBuffer& getUbo()
  {
    return ubo;
  }

  TaskKey taskId() const
  {
    return pack(id, subTask);
  }

  int32_t getIteration() const
  {
    return iteration;
  }

private:
  void determineBufferLifetimes();
  void gatherLeafs(dshandle nodeid, std::unordered_map<int32_t, int>& edgeMap, std::vector<dshandle>& leafs);
  void allocateResources();
  void sortNodes();

  void prepareNodes(dshandle);

  // Images are not shared
  struct Image
  {
    GfxImage2D::handle image;
    ImageFormat        format = ImageFormat::eFloat;
    uint32_t           width  = 0;
    uint32_t           height = 0;
    bool               transient;

    ~Image();
  };

  // Buffers are reused
  struct Buffer
  {
    uint32_t size       = 0;
    int32_t  phyId      = 0;
    int32_t  write      = -1;
    int32_t  read       = -1;
    int32_t  usageCount = 0;
    bool     transient  = false;
  };

  struct TileTask
  {
    EnvParams              envParams;
    std::vector<Image>     images;
    std::vector<Buffer>    buffers;
    std::vector<GpuBuffer> gpuBuffers;
    std::vector<dshandle>  nodesToDelete;
    // all nodes that are executed, and in order
    std::vector<dshandle> tasks;
  };

  std::vector<TileTask> tiles;

  // main actor
  dshandle actor;

  GpuBuffer        ubo;
  LaunchParams     launchParams;
  GfxFence::handle sync;
  int32_t          iteration = 0;
  uint32_t         id        = 0;
  uint32_t         subTask   = 0;
  bool             dirty     = false;
};

} // namespace terra