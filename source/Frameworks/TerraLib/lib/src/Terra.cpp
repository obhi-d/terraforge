#include "Terra.h"
#include "Logger.h"
#include "NodeBuilder.h"

#include <format>
#include <fstream>

neo_registry(NoiseBuilder);
namespace terra
{

Terra Terra::instance;

void Terra::init(std::shared_ptr<RenderDevice> dev, Localization l)
{
  localizationProvider = l;
  device = dev;
  NodeRegister(NoiseBuilder, registry);
}

void Terra::scanShader(std::filesystem::path path)
{
  std::ifstream iff(path);
  if (iff.is_open())
  {
    NodeMeta           newMeta;
    NodeCmdHandler     handler(newMeta,
                               [this](std::string err)
                               {
                             logError(err);
                           });
    neo::state_machine sm{registry, &handler};

    std::string f1_str((std::istreambuf_iterator<char>(iff)), std::istreambuf_iterator<char>());
    sm.parse(path.string(), f1_str);
    if (!sm.fail_bit())
    {
      newMeta.id = path.stem().string();
      newMeta.buildShaderGLSL(handler.content);
      auto it    = metaMap.find(newMeta.id);
      if (it != metaMap.end())
      {
        nodeMetaTable[it->second].destroy();
        nodeMetaTable[it->second] = newMeta;
      }
      else
      {
        uint32_t id = (uint32_t)nodeMetaTable.size();
        metaMap.emplace(newMeta.id, id);
        nodeMetaTable.emplace_back(newMeta);
      }
    }
    else
    {
      sm.for_each_error(
        [](std::string_view err) {
          logError(err);
        });
    }
  }
  else
  {
    logError("Failed to open file: {}", path.string());
  }
}

GfxSampler::handle Terra::getSampler(ImageSampling sampling)
{
  for (size_t i = 0; i < samplers.size(); ++i)
    if (samplers[i].first == sampling)
      return samplers[i].second;
  auto sampler = device->createSampler(sampling);
  if (!sampler)
    logError("Failed to create a sampler.");
  samplers.emplace_back(sampling, sampler);
  return sampler;
}

index<ImageData> Terra::getImage(std::filesystem::path path)
{
  for (uint32_t i = 0; i < images.size(); ++i)
    if (images[i].source == path)
      return index<ImageData>((uint32_t)i);

  return images.emplace(path);
}

hnode Terra::createNode(NodeMeta const& meta)
{
  auto lnk = nodes.emplace(meta);
  nodes.at(lnk).setId(lnk);
  return lnk;
}

uint32_t Terra::getSemantic(std::string_view from)
{
  for (uint32_t i = 0; i < semantics.size(); ++i)
  {
    if (semantics[i] == from)
      return i + 1;
  }
  semantics.emplace_back(from);
  return semantics.size() - 1;
}

} // namespace terra