
#pragma once

#include "GfxDevice.h"
#include "ImageCodec.h"
#include "Node.h"
#include "Table.h"

#include <ThreadPool.h>
#include <WorkerThread.h>
#include <map>
#include <neo_registry.hpp>

namespace terra
{
struct GfxDevice;
struct ShaderBuilder;
class Terra
{
public:
  using Localization = std::function<std::u8string_view(std::string_view)>;
  void init(Localization loc, std::shared_ptr<GfxDevice> iDev);

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
  inline As& get(HDataSource at)
  {
    return static_cast<As&>(*dataSources[at].get());
  }

  template <typename As>
  inline As const& get(HDataSource at) const
  {
    return static_cast<As const&>(*dataSources[at].get());
  }

  inline bool isValid(HDataSource at) const
  {
    return at && dataSources.contains(at) && dataSources.at(at) &&
           get<DataSource>(at).getSelf() == DataSource::handle(at.reserved);
  }

  template <typename As>
  inline As* getIf(HDataSource at) const
  {
    if (at && dataSources.contains(at))
    {
      auto& ptr = dataSources.at(at);
      if (ptr && ptr->getSelf() == at)
        return static_cast<As*>(ptr.get());
    }
    return nullptr;
  }

  inline void destroy(HDataSource n)
  {
    dataSources.erase(n);
  }

  template <typename T>
  inline void replace(T& oldT, T& newT, HDataSource node)
  {
    if (oldT)
      get(oldT);
  }

  HDataSource createNode(NodeMeta const&);
  HDataSource getImage(std::filesystem::path path);
  HDataSource createCurve();

  template <typename Meta>
  void addMeta(std::string_view name, Meta const& meta)
  {
    metaMap[name] = (uint32_t)nodeMetaTable.size();
    nodeMetaTable.push_back(std::static_pointer_cast<NodeMeta>(std::make_shared<Meta>(meta)));
    nodeMetaTable.back()->id = (uint32_t)nodeMetaTable.size();
    nodeMetaTable.back()->prepare();
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

  GfxDevice& getDevice()
  {
    return *device.get();
  }

  void scanShader(std::filesystem::path path);

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
  std::shared_ptr<GfxDevice> device;
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

template <typename Meta>
struct MetaBuilder
{
  std::string_view   name;
  std::u8string_view category;
  std::string_view   style;
  Meta               dummy;

  void clear()
  {
    dummy.parameterDef.clear();
  }

  template <typename NodeT>
  Meta& add(std::string_view name, std::string_view icon)
  {
    this->name = name;
    dummy      = Meta();
    dummy.displayInfo.from(name);
    dummy.icon     = icon;
    dummy.category = category;
    dummy.style    = style;
    dummy.as<NodeT>();
    return dummy;
  }

  void outputs(std::string_view name, DataFormat fmt)
  {
    OutputMeta output(std::string(name));
    output.format = fmt;
    dummy.outputs.emplace_back(output);
  }

  template <typename NodeT>
  Meta& add(NoDomain, std::string_view name, std::string_view icon)
  {
    this->name = name;
    dummy      = Meta(NoDomain{});
    dummy.displayInfo.from(name);
    dummy.icon     = icon;
    dummy.category = category;
    dummy.style    = style;
    dummy.as<NodeT>();
    return dummy;
  }

  template <typename Fn>
  void fn(Fn f)
  {
    dummy.fn = f;
  }

  template <typename Fn>
  void prepare(Fn f)
  {
    dummy.prepare = f;
  }

  template <typename Fn>
  void begin(Fn f)
  {
    dummy.beginIt = f;
  }

  template <typename Fn>
  void end(Fn f)
  {
    dummy.endIt = f;
  }

  void domain()
  {
    dummy.addDomain();
  }

  template <auto M, typename... Args>
  void param(Args... args)
  {
    dummy.parameterDef.emplace_back(MemberPtr<M>(), std::forward<Args>(args)...);
  }

  void done()
  {
    get().addMeta(name, dummy);
  }
};

template <typename Meta>
MetaBuilder<Meta> buildMeta(std::u8string_view cat, std::string_view style)
{
  MetaBuilder<Meta> builder;
  builder.category = cat;
  builder.style    = style;
  return builder;
}

} // namespace terra
