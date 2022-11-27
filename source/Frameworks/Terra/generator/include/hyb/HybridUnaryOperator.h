
#pragma once

#include "HybridBuffer.h"
#include "HybridNode.h"

namespace terra
{

struct HybridUnaryOperator : public ClassicHybridNode
{
  struct Data
  {
    HHybridBuffer buffer;
  };

  enum Operator : int32_t
  {
    eNone,
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

  Operator  op = eNone;
  Parameter offsetScale;
  Parameter source;

  void execute(HybridPipeline&) const override;
};

} // namespace terra