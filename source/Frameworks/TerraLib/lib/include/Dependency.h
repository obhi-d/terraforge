#pragma once
#include <unordered_set>
#include <algorithm>
#include "Common.h"

namespace terra
{
class Terra;
class Node;
class Terra;
class Dependency
{
public:
  Dependency() = default;
  Dependency(Dependency&&) noexcept = default;
  Dependency(Dependency const&) = delete;
  Dependency&  operator=(Dependency&&) noexcept = default;
  Dependency&  operator=(Dependency const&) = delete;
  virtual void add(dshandle node)
  {
    dependents.emplace(node);
  }

  virtual void remove(dshandle node)
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
    std::for_each(dependents.begin(), dependents.end(), lambda);
  }

  static void replace(ghandle o);

private:
  std::unordered_set<dshandle, DSHandleHash> dependents;
};


}