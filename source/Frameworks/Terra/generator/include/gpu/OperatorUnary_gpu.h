
#pragma once

#include "gpu/Node_gpu.h"

namespace terra
{

  class OperatorUnary_gpu : public Node_gpu
  {
    enum class Operator
    {
      eNegate,
      eAbs,
      eSquare,
      eRoot
    };

    Parameter source;
  };
}