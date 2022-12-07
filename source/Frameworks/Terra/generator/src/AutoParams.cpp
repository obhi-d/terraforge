
#include "NodeMeta.h"
#include "Node.h"
#include "Pipeline.h"

namespace terra
{

AutoParam::Result postIteration(Pipeline& pipe, Node& node, uint32_t i) 
{
  auto value = node.param(i);
  if (std::holds_alternative<ScalarValue>(value))
  {
    if (pipe.getIteration() < std::get<ScalarValue>(value).ivalue)
      return AutoParam::eContinueIteration;
  }
  return AutoParam::eOk;
}

void registerAutos() 
{
  NodeMeta::registerAuto(Semantic::eIteration, nullptr, postIteration);
}
}