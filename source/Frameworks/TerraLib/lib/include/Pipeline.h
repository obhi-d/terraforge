
#pragma once

#include "Node.h"

namespace terra
{

class Pipeline
{
public:
private:
  NodePtr              output;
  std::vector<NodePtr> pipeline;
};

} // namespace terra