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
    dependencies.emplace(node);
  }

  virtual void remove(hnode node)
  {
    dependencies.erase(node);
  }

  bool isDetached() const
  {
    return dependencies.empty();
  }

  template <typename L>
  void forEachDependent(L&& lambda) const
  {
    std::for_each(dependencies.begin(), dependencies.end(), lambda);
  }

  void propagate(NodeEvent);

private:
  std::unordered_set<int32_t> dependencies;
};

}