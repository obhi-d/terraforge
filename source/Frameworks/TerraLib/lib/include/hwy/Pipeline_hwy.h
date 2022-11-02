#pragma once

#include "Buffer_hwy.h"
#include "NodeMeta_hwy.h"
#include "Pipeline.h"

#include <atomic>
#include <future>
#include <optional>
#include <semaphore>

namespace terra
{
using hwybuffer = Buffer_hwy<float>;
struct hwyvb
{
  hwybuffer x;
  hwybuffer y;

  auto pitch() const
  {
    return x.pitch();
  }

  hwyvb(uint32_t width, uint32_t height, uint32_t lanes) : x(width, height, lanes), y(width, height, lanes) {}
};

class Pipeline_hwy : public Pipeline
{

public:
  void        getResults(float*, float& min, float& max) final;
  std::size_t hasResults() final;

  hwybuffer& getOutput(uint32_t thread, uint32_t lanes);
  hwybuffer& pushOutput(uint32_t thread, uint32_t lanes);
  void       popOutput(uint32_t thread);

  hwyvb& getInput(uint32_t thread, uint32_t lanes, bool populated = false);
  hwyvb& pushInput(uint32_t thread, uint32_t lanes, bool populated = false);
  void   popInput(uint32_t thread);

protected:
  void wait() final;
  void launch() final;
  void pushTileTask(EnvParams const&) final;

private:
  struct ThreadData
  {
    EnvParams params;
    int32_t   width  = 0;
    int32_t   height = 0;
    vec2      minMax = {-1.0f, 1.0f};

    std::vector<hwyvb>     inputs;
    std::vector<hwybuffer> outputs;
  };

  // tiles are subdivided into NxN blocks of vectors (M lanes)
  static constexpr int32_t N        = 16;
  std::atomic_int          finished = 0;
  std::binary_semaphore    semaphore{1};
  std::vector<ThreadData>  threadDatas;
};

} // namespace terra