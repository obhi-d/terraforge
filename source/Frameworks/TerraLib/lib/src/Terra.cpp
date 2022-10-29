#include "Terra.h"
#include "Logger.h"
#include "ResourceUtils.h"

#include "hwy/Pipeline_hwy.h"
#include "gpu/Pipeline_gpu.h"

#include <format>
#include <fstream>

neo_registry(NoiseBuilder);
namespace terra
{

void Operators_hwy();
void Noise_hwy();

Terra Terra::instance;

void Terra::init(Localization l, std::shared_ptr<ComputeDevice> iDev)
{
  localizationProvider = l;
  device               = iDev;
  pipelineType         = device ? PipelineType::eGPU : PipelineType::eCPU;
  if (pipelineType == PipelineType::eGPU)
  {
    // TODO
    assert(false);
  }
  else
  {
    Operators_hwy();
    Noise_hwy();
  }
}

void Terra::destroy() 
{  
  dataSources.clear();
  nodeMetaTable.clear();
  // computeThread.shutdown();
 }

void Terra::addMeta(std::string name, NodeMeta const& meta)
{
  metaMap[name] = (uint32_t)nodeMetaTable.size();
  nodeMetaTable.push_back(meta);
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
  auto ptr = meta.create(meta);
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

std::shared_ptr<Pipeline> Terra::createPipeline() const 
{
  if (pipelineType == PipelineType::eGPU)
  {
    // TODO
    assert(false);
    return nullptr;
  }
  else
  {
    return std::make_shared<Pipeline_hwy>();
  }
}

} // namespace terra