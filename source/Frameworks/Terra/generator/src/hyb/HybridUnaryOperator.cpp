
#include "hyb/HybridUnaryOperator.h"

namespace terra
{

void HybridUnaryOperator::execute(HybridPipeline& pipe) const
{
  pipe.getCurrentOutput();
}

} // namespace terra