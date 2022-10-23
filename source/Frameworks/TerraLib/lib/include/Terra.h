
#pragma once
#include "ImageCodec.h"
#include "Node.h"
#include "Table.h"

#include <map>
#include <neo_registry.hpp>

namespace terra
{
struct RenderDevice;
struct ShaderBuilder;
class Terra
{
public:

  struct CommonShaderContent
  {
    std::string fixedResources;
    std::string main;
    std::string typesAndConstants;
    std::string utilityFunctions;
  };

  using Localization = std::function<std::u8string_view(std::string_view)>;
  void init(std::shared_ptr<RenderDevice> compute, Localization loc);

  inline void addImageCodec(std::u8string ext, std::shared_ptr<ImageCodec> codec)
  {
    imageCodecs[ext] = codec;
  }

  inline bool isShaderConfigSupported(std::string_view name)
  {
    if (unsupportedShaderConfigs.find(name) != unsupportedShaderConfigs.npos)
      return false;
    return true;
  }

  void scanShader(std::filesystem::path path);

  inline CommonShaderContent const& getShaderContent(ShaderLang) const
  {
    return shaderContent;
  }
  
  inline std::shared_ptr<ImageCodec> getImageCodeFor(std::u8string ext)
  {
    auto it = imageCodecs.find(ext);
    if (it != imageCodecs.end())
      return it->second;
    return {};
  }
  
  template <typename As>
  inline As& get(dshandle at)
  {
    return static_cast<As&>(*dataSources[at].get());
  }

  template <typename As>
  inline As const& get(dshandle at) const
  {
    return static_cast<As const&>(*dataSources[at].get());
  }

  inline bool isValid(dshandle at) const
  {
    return at && dataSources.contains(at) && dataSources.at(at) &&
      get<DataSource>(at).getSelf() == DataSource::handle(at.reserved);
  }

  inline void destroy(dshandle n)
  {
    dataSources.erase(n);
  }


  template <typename T>
  inline void replace(T& oldT, T& newT, dshandle node)
  {
    if (oldT)
      get(oldT);
  }

  dshandle createNode(NodeMeta const&);
  dshandle getImage(std::filesystem::path path);


  GfxSampler::handle getSampler(ImageSampling sampling);

  inline NodeMeta* getNodeMeta(std::string name)
  {
    auto it = metaMap.find(name);
    if (it != metaMap.end())
    {
      return &nodeMetaTable[it->second];
    }
    return {};
  }

  inline RenderDevice& getDevice()
  {
    return *device;
  }

  inline uint32_t frameNumber() const
  {
    return frame;
  }

  void destroy();

  inline static Terra& get()
  {
    return instance;
  }

  inline constexpr uint32_t getWorkGroupSize() const
  {
    return 32;
  }

  Localization localizationProvider;

  template <typename L>
  inline void forEachMeta(L&& l)
  {
    // ordered traversal using map
    for (auto& m : metaMap)
      l(m.first, m.second, nodeMetaTable[m.second]);
  }

  template <typename L>
  inline void forEachNode(L&& l)
  {
    dataSources.for_each(
      [](DataSourcePtr& ds) -> bool
      {
        if (ds->getType() == DataSource::Type::eNode)
          l(*(Node*)ds.get());
      });
  }

  uint32_t getSemantic(std::string_view from);

private:
  static Terra instance;

  using ImageCodecMap = std::unordered_map<std::u8string, std::shared_ptr<ImageCodec>>;
  using SamplerList   = std::vector<std::pair<ImageSampling, GfxSampler::handle>>;

  uint32_t                 frame = 0;
  CommonShaderContent      shaderContent;
  std::vector<std::string> semantics;

  using NodeMetaMap = std::map<std::string, uint32_t>;
  std::vector<NodeMeta>         nodeMetaTable;
  NodeMetaMap                   metaMap;
  std::string                   unsupportedShaderConfigs;
  neo::registry                 registry;
  std::shared_ptr<RenderDevice> device;
  ImageCodecMap                 imageCodecs;
  table<DataSourcePtr>          dataSources;
  SamplerList                   samplers;
};

inline Terra& get()
{
  return Terra::get();
}
} // namespace terra