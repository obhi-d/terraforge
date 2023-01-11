
#include "Node.h"
#include "NodeMeta.h"
#include "Pipeline.h"
#include <fmt/format.h>
#include <numbers>

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
  if (std::holds_alternative<uint32_t>(value))
  {
    if (pipe.iteration() < std::get<uint32_t>(value))
      return AutoParam::eContinueIteration;
  }
  return AutoParam::eReturnResult;
}

AutoParam::Result preFSeed(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = float(pipe.seed() % 100);
  return AutoParam::Result::eOk;
}

AutoParam::Result preSeed(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = pipe.seed();
  return AutoParam::Result::eOk;
}

AutoParam::Result preFrequency(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = pipe.frequency() * std::get<float>(node.param(i));
  return AutoParam::Result::eOk;
}

AutoParam::Result preStart(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = vec2((float)pipe.offset().x + (pipe.size().x * pipe.tile().x),
              (float)pipe.offset().y + (pipe.size().y * pipe.tile().y));
  return AutoParam::Result::eOk;
}

AutoParam::Result preSize(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = vec2((float)pipe.size().x, (float)pipe.size().y);
  return AutoParam::Result::eOk;
}

AutoParam::Result preRecipSize(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = vec2(1.0f / (float)pipe.size().x, 1.0f / (float)pipe.size().y);
  return AutoParam::Result::eOk;
}

AutoParam::Result preMinMax(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  auto val = pipe.minMax();
  pout     = vec2(val.x, 1.0f / (val.y - val.x));
  return AutoParam::Result::eOk;
}

void changeExpOctave(Node& node, uint32_t i)
{
  auto exponentVal = std::get<float>(node.param(Semantic("exponent")));
  auto octaves     = std::get<uint32_t>(node.param(Semantic("octaves")));
  auto lacunarity  = std::get<float>(node.param(Semantic("lacunarity")));
  auto param       = node.param(i);
  if (!std::holds_alternative<ArrayFloatRef>(param))
    param = std::make_shared<ArrayFloat>();
  ArrayFloatRef afr = std::get<ArrayFloatRef>(param);
  afr->resize(16);
  for (uint32_t i = 0, e = std::min<uint32_t>(octaves, 16); i < e; ++i)
    afr->at(i) = std::pow(lacunarity, -float(i) * exponentVal);
  node.state(i, afr);
}

void changeBlurFactor(Node& node, uint32_t i)
{
  auto blurWindow = std::get<int32_t>(node.param(Semantic("blur_window")));
  node.state(i, float(1.0f / ((float(blurWindow) * 2.0f + 1.0f) * (float(blurWindow) * 2.0f + 1.0f))));
}

void changeGaussBlurKernel(Node& node, uint32_t i)
{
  auto param = node.param(i);
  if (!std::holds_alternative<BufferRef>(param))
    param = std::make_shared<GpuBuffer>();
  auto     buffer       = std::get<BufferRef>(param);
  auto     stdDeviation = std::get<float>(node.param(Semantic("blur_stddev")));
  float    sqDeviation  = 2 * stdDeviation * stdDeviation;
  auto     blurWindow   = std::get<int32_t>(node.param(Semantic("blur_window")));
  float    k            = 1.0f / (std::numbers::pi_v<float> * sqDeviation);
  uint32_t count        = (blurWindow * 2 + 1) * (blurWindow * 2 + 1);
  buffer->setSize(count * sizeof(vec3) + 4);
  buffer->ensure();
  auto byteData        = buffer->map(0, count * sizeof(vec3) + 4);
  *(uint32_t*)byteData = count;
  byteData += 4;
  vec3* factor = (vec3*)byteData;
  int   ki     = 0;
  float sum    = 0;
  for (int x = -blurWindow; x <= blurWindow; ++x)
  {
    float nx = float(x);
    for (int y = -blurWindow; y <= blurWindow; ++y)
    {
      float ny     = float(y);
      factor[ki].x = nx;
      factor[ki].y = ny;
      factor[ki].z = k * std::exp(-(nx * nx + ny * ny) / sqDeviation);
      sum += factor[ki].z;
      ki++;
    }
  }
  for (int k = 0; k < ki; ++k)
    factor[k].z /= sum;
  buffer->unmap();
  node.state(i, buffer);
}

AutoParam::Result preparePass(Pipeline& pipe, Node& node, uint32_t p)
{
  if (pipe.iteration() == 0)
    return AutoParam::Result::eOk;
  return AutoParam::eSkipPass;
}

AutoParam::Result prePausePlay(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  auto bs = std::get<Button>(node.param(i));
  pout    = bs;
  if (bs.state)
    return AutoParam::Result::ePauseExecution;
  return AutoParam::Result::eContinueIteration;
}

AutoParam::Result postStopSim(Pipeline& pipe, Node& node, uint32_t i)
{
  auto value = std::get<Button>(node.param(i));
  if (value.state)
  {
    node.state(i, value);
    value.state = false;
    return AutoParam::eReturnResult;
  }
  return AutoParam::Result::eContinueIteration;
}

AutoParam::Result preWater(Pipeline& pipe, Node& node, uint32_t i, Parameter& pout)
{
  pout = node.param(i);
  return AutoParam::Result::eOk;
}

void registerAutos()
{
  NodeMeta::registerAuto(Semantic("iteration"), nullptr, postIteration);
  NodeMeta::registerAuto(Semantic("minmax"), preMinMax, nullptr);
  NodeMeta::registerAuto(Semantic("fseed"), preFSeed, nullptr);
  NodeMeta::registerAuto(Semantic("seed"), preSeed, nullptr);
  NodeMeta::registerAuto(Semantic("frequency"), preFrequency, nullptr);
  NodeMeta::registerAuto(Semantic("start"), preStart, nullptr);
  NodeMeta::registerAuto(Semantic("size"), preSize, nullptr);
  NodeMeta::registerAuto(Semantic("rsize"), preRecipSize, nullptr);
  NodeMeta::registerAuto(Semantic("expoctave"), changeExpOctave,
                         {Semantic("exponent"), Semantic("octaves"), Semantic("lacunarity")});
  NodeMeta::registerAuto(Semantic("blur_factor"), changeBlurFactor, {Semantic("blur_window")});
  NodeMeta::registerAuto(Semantic("gauss_blur_factor"), changeGaussBlurKernel,
                         {Semantic("blur_window"), Semantic("blur_stddev")});
  NodeMeta::registerAuto(Semantic("prepare_pass"), preparePass);
  NodeMeta::registerAuto(Semantic("play_pause"), prePausePlay, nullptr);
  NodeMeta::registerAuto(Semantic("stop"), nullptr, postStopSim);
}
} // namespace terra