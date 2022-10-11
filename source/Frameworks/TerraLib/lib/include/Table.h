#pragma once
#include <cstdint>
#include <type_traits>

namespace terra
{
template <typename T>
class table
{
  static inline constexpr uint32_t k_null_32 = 0;
  static inline constexpr uint32_t mask      = 0x00ffffff;
  static inline constexpr uint32_t lifecycle = 0x01000000;

  struct free_idx
  {
    std::uint32_t unused = k_null_32;
    std::uint32_t valids = 0;
  };

  using vector   = std::vector<T>;
  using freepool = std::vector<std::uint32_t>;

public:

  template <typename L>
  void for_each(L&& lambda)
  {
    std::sort(free_pool.begin(), free_pool.end());
    free_pool.push_back((uint32_t)pool.size());
    uint32_t start = 0;
    for (uint32_t end : free_pool)
    {
      for (uint32_t n = start; n < end; ++n)
        lambda(pool[n]);
      start = end + 1;
    }
    free_pool.pop_back();
  }

  template <typename... Args>
  std::uint32_t emplace(Args&&... args)
  {
    std::uint32_t index = 0;
    
    if (!free_pool.empty())
    {
      index = free_pool.back() + lifecycle;
      free_pool.pop_back();
      pool[index & mask] = std::move(T(std::forward<Args>(args)...));
    }
    else
    {
      if (pool.empty())
        pool.emplace_back();
      index = static_cast<std::uint32_t>(pool.size());
      pool.emplace_back(std::forward<Args>(args)...);
    }
    
    return index;
  }

  void erase(std::uint32_t index)
  {
    
    pool[index & mask] = T();
    free_pool.emplace_back(index);
    
  }

  T& operator[](std::uint32_t i)
  {
    return reinterpret_cast<T&>(pool[i & mask]);
  }

  T const& operator[](std::uint32_t i) const
  {
    return reinterpret_cast<T const&>(pool[i & mask]);
  }

  T& at(std::uint32_t i)
  {
    return reinterpret_cast<T&>(pool[i & mask]);
  }

  T const& at(std::uint32_t i) const
  {
    return reinterpret_cast<T const&>(pool[i & mask]);
  }

  bool contains(std::uint32_t i) const 
  {
    return ((i & mask) < pool.size());
  }

  std::uint32_t size() const
  {
    return static_cast<std::uint32_t>(pool.size() - free_pool.size());
  }

private:
  vector   pool;
  freepool free_pool;
};
}