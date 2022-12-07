
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
  ~Pipeline()
  {
    cleanup();
  }

  void compute(HDataSource, LaunchParams const&, ivec2 start, ivec2 size); // wrap in Modifiers.toR16

  // this function is called once results are available
  virtual bool getResults(GfxImage2D::handle& heights, GfxImage2D::handle& layerContrib) = 0;
  // returns the size of the results available, 0 if no results are available
  virtual bool hasResults() = 0;

  void cancel();

  // Read for the given node
  virtual void cleanup();

  uint32_t getIteration() const
  {
    return iteration;
  }

  bool reissue(HDataSource node)
  {
    iterationRequests++;
    if (!reissueNode)
    {
      reissueNode = node;
      return true;
    }
    return reissueNode == node;
  }

  void resetIteration()
  {
    iterationRequests = 0;
    reissueNode       = {};
  }

  void resetLastIssued()
  {
    reissueNode = {};
  }

  bool wasReissued(HDataSource c)
  {
    return c == reissueNode;
  }

  float origFrequency() const
  {
    return launchParams.frequency;
  }

  int32_t origSeed() const
  {
    return launchParams.seed;
  }

  HDataSource getActor() const
  {
    return actor;
  }

  LaunchParams const& params() const
  {
    return launchParams;
  }

  ivec2 const& launchSize() const
  {
    return size;
  }

  ivec2 const& launchOffset() const
  {
    return start;
  }

  bool isActor(HDataSource h) const
  {
    return h == actor;
  }

  uint32_t getId() const
  {
    return id;
  }

protected:
  void onLaunch()
  {
    iterationRequests = 0;
  }

  bool hasMoreIterations()
  {
    return iterationRequests != 0;
  }

  bool isPrimary(HDataSource h) const
  {
    return actor == h;
  }

  virtual void pushTileTask(EnvParams const&) = 0;
  virtual void launch()                       = 0;

  // main actor
  HDataSource  reissueNode;
  HDataSource  actor;
  uint32_t     iterationRequests = 0;
  LaunchParams launchParams;
  ivec2        start;
  ivec2        size;
  uint32_t     iteration = 0;
  uint32_t     tile      = 0;
  uint32_t     nbTiles   = 0;
  uint32_t     id        = 0;
};

} // namespace terra