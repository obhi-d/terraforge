#pragma once
#include <unordered_set>
#include <algorithm>
#include "Common.h"

namespace terra
{
class Terra;
class Node;
enum class NodeEvent
{
  eValueModified,
  eOptionChanged
};
class Terra;
class Dependency
{
public:
  virtual void add(hnode node)
  {
    dependents.emplace(node);
  }

  virtual void remove(hnode node)
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

  void propagate(NodeEvent);

private:
  std::unordered_set<int32_t> dependents;
};

}