
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
  struct ShaderContent
  {
    std::string fixedResources;
    std::string main;
    std::string typesAndConstants;
    std::string utilityFunctions;
  };

  void init(std::shared_ptr<RenderDevice>);

  void addImageCodec(std::u8string ext, std::shared_ptr<ImageCodec> codec)
  {
    imageCodecs[ext] = codec;
  }

  bool isShaderConfigSupported(std::string_view name)
  {
    if (unsupportedShaderConfigs.find(name) != unsupportedShaderConfigs.npos)
      return false;
    return true;
  }

  void scanShader(std::filesystem::path path);
  void logError(std::string);

  Content              loadMediaContent(std::string media);
  std::string          loadMediaString(std::string media);
  ShaderContent const& getShaderContent(ShaderLang) const
  {
    return shaderContent;
  }
  std::shared_ptr<ImageCodec> getImageCodeFor(std::u8string ext)
  {
    auto it = imageCodecs.find(ext);
    if (it != imageCodecs.end())
      return it->second;
    return {};
  }

  hnode createNode(NodeMeta&);

  index<ImageData>     getImage(std::filesystem::path path);
  ImageData const&     getImage(index<ImageData>) const;
  ImageData&           getImage(index<ImageData>);
  GfxSampler::handle   getSampler(ImageSampling sampling);

  NodeMeta* getNodeMeta(std::string name)
  {
    auto it = metaMap.find(name);
    if (it != metaMap.end())
    {
      return &nodeMetaTable[it->second];
    }
    return {};
  }

  RenderDevice& getDevice()
  {
    return *device;
  }

  Node& getNode(hnode node)
  {
    return nodes.at(node.reserved);
  }

  Node const& getNode(hnode node) const
  {
    return nodes.at(node.reserved);
  }

  void propagate(hnode n, NodeEvent e)
  {
    auto& node = getNode(n);
    if (e == NodeEvent::eValueModified)
      node.markValueChanged();
    else
      node.markOptionChanged();
  }

  uint32_t frameNumber() const
  {
    return frame;
  }

  bool isValid(hnode node) 
  {
    return nodes.contains(node.reserved) && getNode(node).getId() == node;
  }

  void destroy(hnode n) 
  {
    nodes.erase(n.reserved);
  }

  static Terra& get()
  {
    return instance;
  }

  constexpr uint32_t getWorkGroupSize() const
  {
    return 32;
  }

  private:
  
  static Terra instance;

  using ImageCodecMap = std::unordered_map<std::u8string, std::shared_ptr<ImageCodec>>;
  using SamplerList   = std::vector<std::pair<ImageSampling, GfxSampler::handle>>;

  uint32_t      frame = 0;
  ShaderContent shaderContent;
  using NodeMetaMap = std::map<std::string, uint32_t>;
  std::vector<NodeMeta>         nodeMetaTable;
  NodeMetaMap                   metaMap;
  std::string                   unsupportedShaderConfigs;
  neo::registry                 registry;
  std::shared_ptr<RenderDevice> device;
  ImageCodecMap                 imageCodecs;
  table<Node>                   nodes;
  table<ImageData>              images;
  SamplerList                   samplers;
};

inline Terra& get()
{
  return Terra::get();
}
} // namespace terra