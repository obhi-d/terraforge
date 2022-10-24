#include "Terra.h"
#include "Logger.h"
#include "NodeBuilder.h"
#include "ResourceUtils.h"

#include <format>
#include <fstream>

neo_registry(NoiseBuilder);
namespace terra
{

Terra Terra::instance;

void Terra::init(std::shared_ptr<ComputeDevice> dev, Localization l)
{
  localizationProvider = l;
  device = dev;
  NodeRegister(NoiseBuilder, registry);

  shaderContent.fixedResources    = fileContentToString("shaderbuilder/fixed_resources.comp", true);
  shaderContent.main              = fileContentToString("shaderbuilder/main.comp", true);
  shaderContent.typesAndConstants = fileContentToString("shaderbuilder/types_constants.comp", true);
  shaderContent.utilityFunctions  = fileContentToString("shaderbuilder/utility.comp", true);
}

void Terra::destroy() 
{  
  dataSources.clear();
  nodeMetaTable.clear();
  // computeThread.shutdown();
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
        nodeMetaTable[it->second] = std::move(newMeta);
      }
      else
      {
        uint32_t id = (uint32_t)nodeMetaTable.size();
        metaMap.emplace(newMeta.id, id);
        nodeMetaTable.emplace_back(std::move(newMeta));
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

void Terra::addMeta(std::string name, NodeMeta::ShaderContent const& content, NodeMeta&& meta)
{
  metaMap[name] = (uint32_t)nodeMetaTable.size();
  nodeMetaTable.emplace_back(std::move(meta));
  if (nodeMetaTable.back().buildGLSL)
    nodeMetaTable.back().buildGLSL(nodeMetaTable.back(), content);
  else
    nodeMetaTable.back().buildShaderGLSL(content);
}

GfxSampler::handle Terra::getSampler(ImageSampling sampling)
{
  for (size_t i = 0; i < samplers.size(); ++i)
    if (samplers[i].first == sampling)
      return samplers[i].second;
  
  /* auto sampler = computeThread.add(
    [sampling, device = this->device]() -> GfxSampler::handle
    {
      return device->createSampler(sampling);
    });
  sampler.wait();
  GfxSampler::handle result = sampler.get();
  */
  GfxSampler::handle result = device->createSampler(sampling);
  if (!result)
    logError("Failed to create a sampler.");
  samplers.emplace_back(sampling, result);
  return result;
}

dshandle Terra::getImage(std::filesystem::path path)
{
  for (uint32_t i = 0; i < dataSources.size(); ++i)
    if (dataSources[i]->getType() == DataSource::Type::eImage)
      return dataSources[i]->getSelf();

  auto ptr = std::make_shared<Image>(path);
  ptr->setSelf(dataSources.emplace(ptr));
  return ptr->getSelf();
}

dshandle Terra::createNode(NodeMeta const& meta)
{
  auto ptr = std::make_shared<Node>(meta);
  ptr->setSelf(dataSources.emplace(ptr));
  return ptr->getSelf();
}

uint32_t Terra::getSemantic(std::string_view from)
{
  for (uint32_t i = 0; i < semantics.size(); ++i)
  {
    if (semantics[i] == from)
      return i + 1;
  }
  semantics.emplace_back(from);
  return (uint32_t)semantics.size() - 1;
}

} // namespace terra