#pragma once

#include "Buffer_hwy.h"
#include "Common.h"
#include "NodeMeta_hwy.h"
#include "Pipeline.h"

#include <acl/sparse_vector.hpp>
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

  hwyvb(hwyvb&&) noexcept = default;
  hwyvb& operator=(hwyvb&&) noexcept = default;

  hwyvb(hwyvb const& other) noexcept : x(other.x), y(other.y) {}
  hwyvb& operator=(hwyvb const& other) noexcept
  {
    x = other.x;
    y = other.y;
    return *this;
  }


  constexpr hwyvb() = default;
  hwyvb(uint32_t width, uint32_t height, uint32_t lanes) : x(width, height, lanes), y(width, height, lanes) {}
};

class Pipeline_hwy : public Pipeline
{

public:
  struct traits
  {
    using size_type                              = std::uint32_t;
    static constexpr std::uint32_t pool_size     = 8;
    static constexpr std::uint32_t idx_pool_size = 8;
    static constexpr bool          assume_pod_v  = false;
    // null
    // static constexpr T null_v = {};
    // using offset
    // using offset = acl::offset<&selfref::self>;
  };

  struct ThreadData
  {
    EnvParams params;
    int32_t   width  = 0;
    int32_t   height = 0;
    vec2      minMax = {-1.0f, 1.0f};
    uint32_t  thread = 0;
    UVMeter   uv;

    acl::sparse_vector<hwyvb, acl::default_allocator<>, traits>     inputs;
    acl::sparse_vector<hwybuffer, acl::default_allocator<>, traits> outputs;

    ThreadData() = default;
    ThreadData(ThreadData const& other) noexcept : params(other.params), width(other.width), height(other.height) {}
    ThreadData(ThreadData&&) noexcept                 = default;
    ThreadData& operator=(ThreadData const&) noexcept = delete;
    ThreadData& operator=(ThreadData&&) noexcept      = default;
  };

  Pipeline_hwy()                                        = default;
  Pipeline_hwy(Pipeline_hwy&&) noexcept                 = default;
  Pipeline_hwy& operator=(Pipeline_hwy&&) noexcept      = default;
  Pipeline_hwy(Pipeline_hwy const&) noexcept            = delete;
  Pipeline_hwy& operator=(Pipeline_hwy const&) noexcept = delete;

  void        getResults(float*, uint32_t size, float& min, float& max) final;
  std::size_t hasResults() final;

  hwybuffer& getOutput(uint32_t thread, uint32_t lanes);
  hwybuffer& pushOutput(uint32_t thread, uint32_t lanes);
  void       popOutput(uint32_t thread);

  hwyvb& getInput(uint32_t thread, uint32_t lanes, bool populated = false);
  hwyvb& pushInput(uint32_t thread, uint32_t lanes, bool populated = false);
  void   popInput(uint32_t thread);

  ThreadData const& getThreadData(uint32_t t)
  {
    return threadDatas[t];
  }

protected:
  void wait() final;
  void launch() final;
  void pushTileTask(EnvParams const&) final;

private:
  
  WaitList waiters;

  // tiles are subdivided into NxN blocks of vectors (M lanes)
  static constexpr int32_t N = 16;
  std::vector<ThreadData>  threadDatas;
};

} // namespace terra
