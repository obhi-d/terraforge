
#include "Terra.h"
#include "CurveData.h"
#include "Logger.h"
#include "ResourceUtils.h"

#include "hyb/GpuScriptNodeBuilder.h"
#include "hyb/HybridPipeline.h"

#include <fmt/format.h>
#include <fstream>
#include <mimalloc-2.0/mimalloc-new-delete.h>

neo_registry(GpuScript);
namespace terra
{

// void Operators_hwy();
// void Noise_hwy();
// void Basics_hwy();
// void Domain_hwy();
// void PostProcess_hwy();

Terra Terra::instance;

void Terra::init(Localization l, std::shared_ptr<GfxDevice> iDev)
{
  localizationProvider = l;
  device               = iDev;
  NodeRegister(GpuScript, registry);
}

void Terra::destroy()
{
  dataSources.clear();
  nodeMetaTable.clear();
  threadPool.shutdown();
  // computeThread.shutdown();
}

void Terra::scanShader(std::filesystem::path path)
{
  std::ifstream iff(path);
  if (iff.is_open())
  {
    GpuScriptNodeMeta    newMeta;
    GpuScriptNodeBuilder handler(newMeta,
                                 [this](std::string err)
                                 {
                                   logError(err);
                                 });
    neo::state_machine   sm{registry, &handler};

    std::string f1_str((std::istreambuf_iterator<char>(iff)), std::istreambuf_iterator<char>());
    sm.parse(path.string(), f1_str);
    if (!sm.fail_bit())
    {
      addMeta(path.stem().string(), newMeta);
    }
  }
  else
  {
    logError(std::format("Failed to open file: {}", path.string()));
  }
}
HDataSource Terra::getImage(std::filesystem::path path)
{
  HDataSource found;
  dataSources.for_each(
    [&found, &path](auto& image)
    {
      if (image && image->getType() == DataSource::Type::eImage)
      {
        if (static_cast<Image const*>(image.get())->source == path)
        {
          found = image->getSelf();
          return false;
        }
      }
      return true;
    });
  if (found)
    return found;
  auto ptr = std::make_shared<Image>(path);
  ptr->setSelf(dataSources.emplace(ptr));
  return ptr->getSelf();
}

HDataSource Terra::createNode(NodeMeta const& meta)
{
  auto ptr = meta.createNode(meta);
  ptr->setSelf(dataSources.emplace(ptr));
  return ptr->getSelf();
}

HDataSource Terra::createCurve()
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
  return std::make_shared<HybridPipeline>();
}

void DisplayInfo::from(std::string_view iname)
{
  name = get().localizationProvider(iname);
  std::string tt{iname};
  tt += ".help";
  help = get().localizationProvider(tt);
  tt   = iname;
  tt += ".tip";
  tooltip = get().localizationProvider(tt);
  tt      = iname;
  tt += ".category";
  category = get().localizationProvider(tt);
}

} // namespace terra
