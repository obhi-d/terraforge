
#pragma once

#include "Common.h"
#include "Dependency.h"
#include "RenderResource.h"
#include "Serializer.h"
#include <unordered_set>

namespace terra
{
class Node;
class Pipeline;

using SourceSet = std::unordered_set<Source, SourceHash>;
class DataSource : public Dependency
{
public:
  enum class Type
  {
    eImage,
    eCurve,
    eNode,
  };

  enum class Event
  {
    eValueModified,
    eNodeDeleted
  };

  using handle = HDataSource;

  DataSource() = default;
  DataSource(handle n) : self(n) {}
  virtual ~DataSource()
  {
    propagate(Event::eNodeDeleted);
    self = {};
  }

  // Option
  // virtual bool isEnabled(Pipeline const&) const = 0;
  // Ensure data exists in consumable form (ex. In the GPU)
  // This is always called from GPU-thread
  // So proper sync is expected between data accessed here and main thread
  // virtual bool       ensure(Pipeline&) = 0;

  virtual Type       getType() const                         = 0;
  virtual DataFormat getFormat(uint32 outputIndex = 0) const = 0;

  // Accept the change event from a source, for a data source that is dependent
  virtual void accept(Source source, Event);

  virtual bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
  {
    if (!getFromDataStream(dataStream, serialIdx, self.reserved))
      return false;
    return fromDataStreamImpl(dataStream, serialIdx);
  }

  virtual void toDataStream(std::vector<uint8_t>& dataStream) const
  {
    addToDataStream(dataStream, self.reserved);
    toDataStreamImpl(dataStream);
  }

  auto setSelf(handle h)
  {
    std::swap(self.reserved, h.reserved);
    return h;
  }

  auto getSelf() const
  {
    return self;
  }

  auto getVersion() const
  {
    return version;
  }

  void getSources(SourceSet& set) const
  {
    if (set.contains(Source(self)))
      return;
    set.emplace(Source(self));
    getSourcesImpl(set);
  }

  virtual HelpInfo getHelpInfo(HelpType, int param = -1) const = 0;

  virtual void prepareGeneration(Pipeline&) {}
  virtual void beginIteration(Pipeline&) {}
  virtual void endIteration(Pipeline&) {}

  static void prepareGeneration(HDataSource, Pipeline&);
  static void beginIteration(HDataSource, Pipeline&);
  static void endIteration(HDataSource, Pipeline&);

  void propagate(Event);
  void onParamChange(uint32_t i, Source oldValue, Source newValue);
  bool setParamSource(uint32_t paramIdx, Source);

  static bool isValid(HDataSource);
  static bool isNode(HDataSource);
  using exchange = std::pair<Source, bool>;

  static inline bool isWithinTile(uvec2 tile, uvec2 tileConstraintOffset, uvec2 tileConstraintSize)
  {
    return ((tileConstraintSize[0] == 0) ||
            (tileConstraintOffset[0] >= tile[0] && tile[0] < (tileConstraintOffset[0] + tileConstraintSize[0]))) &&
           ((tileConstraintSize[1] == 0) ||
            (tileConstraintOffset[1] >= tile[1] && tile[1] < (tileConstraintOffset[1] + tileConstraintSize[1])));
  }

  inline void updateVersion()
  {
    ++version;
    propagate(Event::eValueModified);
  }

protected:
  virtual void        onParamChangeImpl(uint32_t i, Source oldValue, Source newValue) {}
  virtual void        getSourcesImpl(SourceSet&) const = 0;
  inline virtual bool fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
  {
    return true;
  }
  inline virtual void toDataStreamImpl(std::vector<uint8_t>& dataStream) const {}
  virtual exchange    setParamSourceImpl(uint32_t paramIdx, Source)
  {
    return exchange();
  }

  HDataSource self;
  uint32_t    version = 0;
};

} // namespace terra