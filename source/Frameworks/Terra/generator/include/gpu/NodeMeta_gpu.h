#pragma once

#include "NodeMeta.h"

namespace terra
{

class Pipeline_gpu;
class NodeMeta_gpu : public NodeMeta
{

  virtual bool isCompute() const            = 0;
  virtual void execute(Pipeline_gpu&) const = 0;
};

} // namespace terra