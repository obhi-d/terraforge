
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

  virtual void actor(HDataSource);
  virtual void seed(uint32_t);
  virtual void frequency(float freq);
  virtual void size(uvec2);
  virtual void offset(ivec2);

  // Tick
  virtual void tick() = 0;
  // Compute
  virtual void compute(uvec2 tile)
  {
    tile_ = tile;
  } 
  // wrap in Modifiers.toR16
  // this function is called once results are available
  virtual void getResults(GfxImage::handle& heights, GfxImage::handle& layerContrib) = 0;
  // returns the size of the results available, 0 if no results are available
  virtual bool hasResults() = 0;
  // Read for the given node
  virtual void cleanup();

  void cancel();

  uint32_t iteration() const
  {
    return iteration_;
  }

  uint32_t seed() const
  {
    return seed_;
  }

  HDataSource actor() const
  {
    return actor_;
  }

  uvec2 const& size() const
  {
    return size_;
  }

  ivec2 const& offset() const
  {
    return offset_;
  }

  uint32_t id() const
  {
    return id_;
  }

  uvec2 tile() const
  {
    return tile_;
  }

protected:
  // main actor
  ivec2       offset_    = ivec2(0);
  uvec2       size_      = ivec2(0);
  HDataSource actor_     = HDataSource();
  float       frequency_ = 0.f;
  uint32_t    seed_      = 0;
  uint32_t    iteration_ = 0;
  uvec2       tile_      = uvec2(0);
  uint32_t    id_        = 0;
  uint32_t    version_   = 0xffffffff;
  uint32_t    cversion_  = 0xffffffff;
};

} // namespace terra