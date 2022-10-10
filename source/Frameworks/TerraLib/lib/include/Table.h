#pragma once
#include <cstdint>
#include <type_traits>

namespace terra
{
template <typename T, bool IsPOD = std::is_trivial_v<T>>
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
  using freepool = std::conditional_t<IsPOD, free_idx, std::vector<std::uint32_t>>;

public:
  template <typename... Args>
  std::uint32_t emplace(Args&&... args)
  {
    std::uint32_t index = 0;
    if constexpr (IsPOD)
    {
      if (free_pool.unused != k_null_32)
      {
        index            = free_pool.unused + lifecycle;
        free_pool.unused = reinterpret_cast<std::uint32_t&>(pool[free_pool.unused & mask]);
      }
      else
      {
        index = pool.empty() ? 1 : static_cast<std::uint32_t>(pool.size());
        pool.resize(index + 1);
      }
      pool[index & mask] = T(std::forward<Args>(args)...);
      free_pool.valids++;
    }
    else
    {
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
    }
    return index;
  }

  void erase(std::uint32_t index)
  {
    if constexpr (IsPOD)
    {
      reinterpret_cast<std::uint32_t&>(pool[index & mask]) = free_pool.unused;
      free_pool.unused                              = index;
      free_pool.valids--;
    }
    else
    {
      pool[index & mask] = T();
      free_pool.emplace_back(index);
    }
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
    if constexpr (IsPOD)
      return free_pool.valids;
    else
      return static_cast<std::uint32_t>(pool.size() - free_pool.size());
  }

private:
  vector   pool;
  freepool free_pool;
};
}