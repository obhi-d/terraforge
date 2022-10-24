#pragma once
#include <cstdint>
#include <type_traits>
#include "Common.h"

namespace terra
{
template <typename T>
class table
{
  
  using handle   = terra::handle<T>;
  using vector   = std::vector<T>;
  using freepool = std::vector<handle>;

public:

  void clear() 
  {
    free_pool.clear();
    pool.clear();
  }

  template <typename L>
  void for_each(L&& lambda)
  {
    std::sort(free_pool.begin(), free_pool.end(),
              [](handle first, handle second)
              {
                return first.index() < second.index();
              });

    free_pool.push_back((uint32_t)pool.size());

    uint32_t start = 1;
    for (auto const& entry : free_pool)
    {
      auto end = entry.index();
      for (uint32_t n = start; n < end; ++n)
      {
        if (!lambda(pool[n]))
        {
          free_pool.pop_back();
          return;
        }
      }
      start = end + 1;
    }
    free_pool.pop_back();    
  }

  template <typename... Args>
  handle emplace(Args&&... args)
  {
    handle index;
    
    if (!free_pool.empty())
    {
      index = free_pool.back().cycle_up();
      free_pool.pop_back();
      pool[index.index()] = std::move(T(std::forward<Args>(args)...));
    }
    else
    {
      if (pool.empty())
        pool.emplace_back();
      index = handle((uint32_t)pool.size());
      pool.emplace_back(std::forward<Args>(args)...);
    }
    
    return index;
  }

  void erase(handle index)
  {
    assert(std::find(free_pool.begin(), free_pool.end(), index) == free_pool.end());
    pool[index.index()] = T();
    free_pool.emplace_back(index);
  }

  T& operator[](handle i)
  {
    return reinterpret_cast<T&>(pool[i.index()]);
  }

  T const& operator[](handle i) const
  {
    return reinterpret_cast<T const&>(pool[i.index()]);
  }

  T& at(handle i)
  {
    return reinterpret_cast<T&>(pool[i.index()]);
  }

  T const& at(handle i) const
  {
    return reinterpret_cast<T const&>(pool[i.index()]);
  }

  bool contains(handle i) const 
  {
    return i.index() < pool.size();
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