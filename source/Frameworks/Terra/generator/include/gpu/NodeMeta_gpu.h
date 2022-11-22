#pragma once

#include "NodeMeta.h"

namespace terra
{

  class Pipeline_gpu;
  class NodeMeta_gpu : public NodeMeta
  {
    enum class Queue
    {
      eCPU,
      eCompute,
      eGraphics
    };

    virtual Queue getQueue() const = 0;
  };

} // namespace terra