
#pragma once
#include "ImageCodec.h"
#include "Node.h"
#include <map>
#include <neo_registry.hpp>

namespace terra
{
struct RenderDevice;
class Terra
{
public:
  struct ShaderBuilder
  {
    std::string blockPacking; // std140 or std430
    std::string glslVersion;
    std::string fixedResources;
    std::string main;
    std::string typesAndConstants;
    std::string utilityFunctions;
  };

  void init(std::shared_ptr<RenderDevice>);

  void addImageCodec(std::string ext, std::shared_ptr<ImageCodec> codec)
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
  ShaderBuilder const& getShaderBuilder() const
  {
    return shaderBuilder;
  }

private:
  using NodeMetaMap   = std::map<std::u8string, std::map<std::u8string, NodeMeta>>;
  using ImageCodecMap = std::unordered_map<std::string, std::shared_ptr<ImageCodec>>;
  ShaderBuilder                 shaderBuilder;
  std::string                   unsupportedShaderConfigs;
  NodeMetaMap                   meta;
  neo::registry                 registry;
  std::shared_ptr<RenderDevice> device;
  ImageCodecMap                 imageCodecs;
};
} // namespace terra