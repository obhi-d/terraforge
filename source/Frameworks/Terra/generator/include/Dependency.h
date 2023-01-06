#pragma once
#include "Common.h"
#include <algorithm>
#include <unordered_set>

namespace terra
{
class Terra;
class Node;
class Terra;
class Dependency
{
public:
  Dependency()                                  = default;
  Dependency(Dependency&&) noexcept             = default;
  Dependency(Dependency const&)                 = delete;
  Dependency&  operator=(Dependency&&) noexcept = default;
  Dependency&  operator=(Dependency const&)     = delete;
  virtual void add(Source node)
  {
    dependents.emplace(node);
  }

  virtual void remove(Source node)
  {
    dependents.erase(node);
  }

  bool isDetached() const
  {
    return dependents.empty();
  }

  template <typename L>
  void forEachDependent(L&& lambda) const
  {
    if (dependents.empty())
      return;
    std::unordered_set<Source, SourceHash> copy = dependents;
    std::for_each(copy.begin(), copy.end(), lambda);
  }

  uint32_t countDependents(uint32_t outIdx) const
  {
    uint32_t count = 0;
    for (auto& s : dependents)
    {
      if (s.secondary == outIdx)
        count++;
    }
    return count;
  }

private:
  std::unordered_set<Source, SourceHash> dependents;
};

} // namespace terra