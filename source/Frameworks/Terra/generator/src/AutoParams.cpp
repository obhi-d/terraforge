
#include "Node.h"
#include "NodeMeta.h"
#include "Pipeline.h"

namespace terra
{
std::unordered_map<std::string, uint16_t> Semantic::semanticMap;
Semantic                                  Semantic::heights("heights");
Semantic                                  Semantic::water("water");
Semantic                                  Semantic::rocks("rocks");
Semantic                                  Semantic::vegetation("vegetation");

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
  pout = ScalarValue(float(pipe.seed() % 100));
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

bool preRecipSize(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = ScalarValue(vec2(1.0f / (float)pipe.size().x, 1.0f / (float)pipe.size().y));
  return true;
}

bool preExpOctave(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  auto        exponentVal = std::get<ScalarValue>(node.param("exponent")).value;
  auto        octaves     = std::get<ScalarValue>(node.param("octaves")).uvalue;
  auto        lacunarity  = std::get<ScalarValue>(node.param("lacunarity")).value;
  ScalarValue sv;
  for (uint32_t i = 0, e = std::min<uint32_t>(octaves, 16); i < e; ++i)
    sv.value16[i] = std::pow(lacunarity, -float(i) * exponentVal);
  pout = sv;
  return true;
}

bool preBlurFactor(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  auto blurWindow = std::get<ScalarValue>(node.param("blur_window")).uvalue;
  pout            = ScalarValue(1.0f / ((float(blurWindow) * 2.0f + 1.0f) * (float(blurWindow) * 2.0f + 1.0f)));
  return true;
}

void registerAutos()
{
  NodeMeta::registerAuto(Semantic("iteration"), nullptr, postIteration);
  NodeMeta::registerAuto(Semantic("fseed"), preFSeed, nullptr);
  NodeMeta::registerAuto(Semantic("seed"), preSeed, nullptr);
  NodeMeta::registerAuto(Semantic("frequency"), preFrequency, nullptr);
  NodeMeta::registerAuto(Semantic("start"), preStart, nullptr);
  NodeMeta::registerAuto(Semantic("size"), preSize, nullptr);
  NodeMeta::registerAuto(Semantic("rsize"), preRecipSize, nullptr);
  NodeMeta::registerAuto(Semantic("expoctave"), preExpOctave, nullptr);
  NodeMeta::registerAuto(Semantic("blur_factor"), preBlurFactor, nullptr);
}
} // namespace terra