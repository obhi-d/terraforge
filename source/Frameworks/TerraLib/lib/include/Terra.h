
#pragma once

#include "ComputeDevice.h"
#include "ImageCodec.h"
#include "Node.h"
#include "Table.h"

#include <ThreadPool.h>
#include <WorkerThread.h>
#include <map>
#include <neo_registry.hpp>

namespace terra
{
struct ComputeDevice;
struct ShaderBuilder;
class Terra
{
public:
  using Localization = std::function<std::u8string_view(std::string_view)>;
  void init(Localization loc, std::shared_ptr<ComputeDevice> iDev);

  inline void addImageCodec(std::string ext, std::shared_ptr<ImageCodec> codec)
  {
    imageCodecs[ext] = codec;
  }

  inline std::shared_ptr<ImageCodec> getImageCodeFor(std::string ext)
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
  dshandle createCurve();

  template <typename Meta>
  void addMeta(std::string_view name, Meta const& meta)
  {
    metaMap[name] = (uint32_t)nodeMetaTable.size();
    nodeMetaTable.push_back(std::static_pointer_cast<NodeMeta>(std::make_shared<Meta>(meta)));
    nodeMetaTable.back()->displayInfo.from(name);
  }

  inline NodeMeta* getNodeMeta(std::string_view name)
  {
    auto it = metaMap.find(name);
    if (it != metaMap.end())
    {
      return nodeMetaTable[it->second].get();
    }
    return {};
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
      l(m.first, m.second, *nodeMetaTable[m.second].get());
  }

  template <typename L>
  inline void forEachNode(L&& l)
  {
    dataSources.for_each(
      [&l](DataSourcePtr& ds) -> bool
      {
        if (ds->getType() == DataSource::Type::eNode)
        {
          if (!l(*(Node*)ds.get()))
            return false;
        }
        return true;
      });
  }

  uint32_t getSemantic(std::string_view from);

  ThreadPool& pool()
  {
    return threadPool;
  }

  std::shared_ptr<Pipeline> createPipeline() const;

private:
  static Terra instance;

  using ImageCodecMap = std::unordered_map<std::string, std::shared_ptr<ImageCodec>>;

  uint32_t                 frame = 0;
  std::vector<std::string> semantics;

  using NodeMetaMap = std::map<std::string_view, uint32_t>;
  std::vector<std::shared_ptr<NodeMeta>> nodeMetaTable;
  NodeMetaMap                            metaMap;
  neo::registry                          registry;
  ImageCodecMap                          imageCodecs;
  table<DataSourcePtr>                   dataSources;
  ThreadPool                             threadPool;
  // WorkerThread computeThread;
  std::shared_ptr<ComputeDevice> device;
  PipelineType                   pipelineType = PipelineType::eCPU;
};

inline Terra& get()
{
  return Terra::get();
}

inline std::u8string_view operator""_ls(const char* input, std::size_t len)
{
  return get().localizationProvider(std::string_view(input, len));
}

inline const char* operator""_lsc(const char* input, std::size_t len)
{
  return (const char*)get().localizationProvider(std::string_view(input, len)).data();
}
} // namespace terra
