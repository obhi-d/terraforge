
#pragma once

#include "HybridNode.h"

namespace terra
{

struct HybridUnaryOperator : public ClassicHybridNode
{
  enum Operator
  {
    eAbs,
    eExp,
    eLn,
    eNegate,
    eRecip,
    eSquare
  };

  Parameter source;

  virtual void execute(HybridPipeline&) const;
};

} // namespace terra