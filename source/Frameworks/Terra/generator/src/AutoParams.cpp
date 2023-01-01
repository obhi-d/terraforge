
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

bool preFSeed(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue(pipe.seed() / 10000.f);
  return true;
}

bool preSeed(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue((int)pipe.seed());
  return true;
}

bool preFrequency(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue(pipe.frequency() * std::get<ScalarValue>(node.param(i)).value);
  return true;
}

bool preStart(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue(vec2((float)pipe.offset().x + (pipe.size().x * pipe.tile().x),
                          (float)pipe.offset().y + (pipe.size().y * pipe.tile().y)));
  return true;
}

bool preSize(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue(vec2((float)pipe.size().x, (float)pipe.size().y));
  return true;
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