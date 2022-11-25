
#pragma once

#include "HybridNode.h"

namespace terra
{

struct HybridUnaryOperator : public ClassicHybridNode
{
  enum Operator : int32_t
  {
    eAbs,
    eExp,
    eLn,
    eNegate,
    eRecip,
    eSquare,
    eRoot,
    eSin,
    eCos
  };

  Operator  op;
  Parameter offsetScale;
  Parameter source;

  void execute(HybridPipeline&) const override;
};

} // namespace terra