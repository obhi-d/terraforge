
#include "Node.h"
#include "NodeMeta.h"
#include "Pipeline.h"

namespace terra
{

AutoParam::Result postIteration(Pipeline& pipe, Node& node, uint32_t i)
{
  auto value = node.param(i);
  if (std::holds_alternative<ScalarValue>(value))
  {
    if (pipe.iteration() < (uint32_t)std::get<ScalarValue>(value).ivalue)
      return AutoParam::eContinueIteration;
  }
  return AutoParam::eOk;
}

AutoParam::Result preFSeed(Pipeline& pipe, Node& node, uint32_t i)
{
  node.param(i, ScalarValue(pipe.seed() / 10000.f));
  return AutoParam::eOk;
}

AutoParam::Result preSeed(Pipeline& pipe, Node& node, uint32_t i)
{
  node.param(i, ScalarValue((int)pipe.seed()));
  return AutoParam::eOk;
}

AutoParam::Result preFrequency(Pipeline& pipe, Node& node, uint32_t i)
{
  node.param(i, ScalarValue(pipe.frequency()));
  return AutoParam::eOk;
}

AutoParam::Result preStart(Pipeline& pipe, Node& node, uint32_t i)
{
  node.param(i, ScalarValue(vec2((float)pipe.offset().x, (float)pipe.offset().y)));
  return AutoParam::eOk;
}

AutoParam::Result preSize(Pipeline& pipe, Node& node, uint32_t i)
{
  node.param(i, ScalarValue(vec2((float)pipe.size().x, (float)pipe.size().y)));
  return AutoParam::eOk;
}

void registerAutos()
{
  NodeMeta::registerAuto(Semantic::eIteration, nullptr, postIteration);
  NodeMeta::registerAuto(Semantic::eFSeed, preFSeed, nullptr);
  NodeMeta::registerAuto(Semantic::eSeed, preSeed, nullptr);
  NodeMeta::registerAuto(Semantic::eFrequency, preFrequency, nullptr);
  NodeMeta::registerAuto(Semantic::eStart, preStart, nullptr);
  NodeMeta::registerAuto(Semantic::eSize, preSize, nullptr);
}
} // namespace terra