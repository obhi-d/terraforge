
#pragma once

#include "Node.h"
#include "gpu/NodeMeta_gpu.h"

namespace terra
{
  struct Node_gpu : public Node
  {
    virtual bool isCompute() const = 0;
    virtual void execute(Pipeline_gpu&) const = 0;
  };

}