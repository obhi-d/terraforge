#pragma once

#include "Buffer_hwy.h"
#include "Common.h"
#include "NodeMeta_hwy.h"
#include "Pipeline.h"

#include <acl/blackboard.hpp>
#include <acl/sparse_vector.hpp>
#include <atomic>
#include <future>
#include <optional>
#include <semaphore>

namespace terra
{

struct PermuatationConstants
{
  std::array<int32_t, 256>                      perm;
  static std::array<std::array<int32_t, 4>, 64> simplexlut;
  static std::array<std::array<float, 3>, 16>   grad3u;
  static std::array<std::array<float, 3>, 16>   grad3v;
  uint64_t                                      seed;
};

class Pipeline_hwy : public Pipeline
{

public:
  struct ThreadData
  {
    EnvParams params;
    int32_t   width  = 0;
    int32_t   height = 0;
    vec2      minMax = {-1.0f, 1.0f};
    uint32_t  thread = 0;
    UVMeter   uv;

    hwyvb_list     inputs;
    hwybuffer_list outputs;

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

  template <typename T>
  T& addCacheData(dshandle ds)
  {
    auto l = cacheData.emplace<T>(ds.um_index());
    return cacheData.at<T>(l);
  }

  template <typename T>
  T const& getCacheData(dshandle ds) const
  {
    return cacheData.at<T>(ds.um_index());
  }

  template <typename T>
  T& getCacheData(dshandle ds) 
  {
    return cacheData.at<T>(ds.um_index());
  }

  ThreadData const& getThreadData(uint32_t t)
  {
    return threadDatas[t];
  }

  PermuatationConstants const& getConstants() const
  {
    return constants;
  }

  uint32_t getNumThreads() const
  {
    return (uint32_t)threadDatas.size();
  }

protected:
  void cleanup() final;
  void wait() final;
  void launch() final;
  void pushTileTask(EnvParams const&) final;

private:
  using CacheMap = std::unordered_map<uint32_t, acl::vlink>;
  PermuatationConstants     constants;
  WaitList                  waiters;
  acl::blackboard<CacheMap> cacheData;
  // tiles are subdivided into NxN blocks of vectors (M lanes)
  static constexpr int32_t N = 32;
  std::vector<ThreadData>  threadDatas;
};

} // namespace terra
