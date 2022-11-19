
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

  void compute(dshandle, LaunchParams const&, ivec2 start, ivec2 size); // wrap in Modifiers.toR16

  // this function is called once results are available
  virtual void getResults(float*, size_t nbFloats, float& min, float& max) = 0;
  // returns the size of the results available, 0 if no results are available
  virtual std::size_t hasResults() = 0;

  void cancel();

  // Read for the given node
  virtual void cleanup();

  int32_t getIteration() const
  {
    return iteration;
  }

  bool reissue(dshandle node)
  {
    continueIter = true;
    if (!reissueNode)
    {
      reissueNode = node;
      return true;
    }
    return false;
  }

  bool wasReissued(dshandle c)
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

  dshandle getActor() const
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

  bool isActor(dshandle h) const
  {
    return h == actor;
  }

protected:
  void onLaunch()
  {
    continueIter = false;
  }

  bool hasMoreIterations()
  {
    return continueIter;
  }

  bool isPrimary(dshandle h) const
  {
    return actor == h;
  }

  virtual void wait()                         = 0;
  virtual void launch()                       = 0;
  virtual void pushTileTask(EnvParams const&) = 0;

  // main actor
  dshandle     reissueNode;
  dshandle     actor;
  bool         continueIter = false;
  LaunchParams launchParams;
  ivec2        start;
  ivec2        size;
  int32_t      iteration = 0;
  uint32_t     tile      = 0;
  uint32_t     nbTiles   = 0;
};

} // namespace terra