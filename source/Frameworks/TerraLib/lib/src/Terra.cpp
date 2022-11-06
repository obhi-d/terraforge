
#include "Terra.h"
#include "Logger.h"
#include "ResourceUtils.h"
#include "CurveData.h"

#include "hwy/Pipeline_hwy.h"
#include "gpu/Pipeline_gpu.h"

#include <fmt/format.h>
#include <fstream>
#include <mimalloc-2.0/mimalloc-new-delete.h>

neo_registry(NoiseBuilder);
namespace terra
{

void Operators_hwy();
void Noise_hwy();
void Basics_hwy();

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
    Basics_hwy();
  }
}

void Terra::destroy() 
{  
  dataSources.clear();
  nodeMetaTable.clear();
  threadPool.shutdown();
  // computeThread.shutdown();
 }


dshandle Terra::getImage(std::filesystem::path path)
{
  for (uint32_t i = 0; i < dataSources.size(); ++i)
    if (dataSources[i]->getType() == DataSource::Type::eImage)
    {
      if (static_cast<Image const*>(dataSources[i].get())->source == path)
        return dataSources[i]->getSelf();
    }
      
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

dshandle Terra::createCurve()
{
  auto ptr = std::make_shared<CurveData>();
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

void DisplayInfo::from(std::string_view iname) 
{
  name = get().localizationProvider(iname);
  std::string tt{iname};
  tt += ".help";
  help = get().localizationProvider(tt);
  tt = iname;
  tt += ".tip";
  tooltip = get().localizationProvider(tt);
}

} // namespace terra
