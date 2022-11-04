#pragma once

#include "Buffer_hwy.h"
#include "Common.h"
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
  Pipeline_hwy()                                        = default;
  Pipeline_hwy(Pipeline_hwy&&) noexcept                 = default;
  Pipeline_hwy& operator=(Pipeline_hwy&&) noexcept      = default;
  Pipeline_hwy(Pipeline_hwy const&) noexcept            = delete;
  Pipeline_hwy& operator=(Pipeline_hwy const&) noexcept = delete;

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
    uint32_t  thread = 0;

    std::vector<hwyvb>     inputs;
    std::vector<hwybuffer> outputs;

    ThreadData() = default;
    ThreadData(ThreadData const& other) noexcept : params(other.params), width(other.width), height(other.height) {}
    ThreadData(ThreadData&&) noexcept                 = default;
    ThreadData& operator=(ThreadData const&) noexcept = delete;
    ThreadData& operator=(ThreadData&&) noexcept      = default;
  };

  WaitList waiters;

  // tiles are subdivided into NxN blocks of vectors (M lanes)
  static constexpr int32_t N = 16;
  std::vector<ThreadData>  threadDatas;
};

} // namespace terra
