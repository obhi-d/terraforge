
#pragma once

#include "Common.h"
#include "GpuBuffer.h"
#include "NodeMeta.h"
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
  virtual void getResults(float*, float& min, float& max) = 0;
  // returns the size of the results available, 0 if no results are available
  virtual std::size_t hasResults()       = 0;

  void cancel();

  // Read for the given node
  void cleanup();

  int32_t getIteration() const
  {
    return iteration;
  }

  bool reissue(dshandle);

  float frequency() const
  {
    return launchParams.frequency;
  }

  int32_t seed() const
  {
    return launchParams.seed;
  }

protected:
  dshandle getActor() const
  {
    return actor;
  }

  LaunchParams const& params() const
  {
    return launchParams;
  }

  uvec2 const& launchSize() const
  {
    return size;
  }

  uvec2 const& launchOffset() const
  {
    return start;
  }

  dshandle updateActor() 
  {
    if (reissued)
      actor = reissued;
    else
      actor = {}; 
    return actor;
  }

  virtual void wait()                         = 0;
  virtual void launch()                       = 0;
  virtual void pushTileTask(EnvParams const&) = 0;

private:
  // main actor
  dshandle     actor;
  dshandle     reissued;
  LaunchParams launchParams;
  uvec2        start;
  uvec2        size;
  int32_t      iteration = 0;
  uint32_t     tile      = 0;
  uint32_t     nbTiles   = 0;
};

} // namespace terra