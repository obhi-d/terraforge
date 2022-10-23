
#pragma once

#include "Common.h"
#include "Dependency.h"
#include "RenderResource.h"
#include "Serializer.h"

namespace terra
{
class Node;
class Pipeline;
class DataSource : public Dependency
{
public:
  enum class Type
  {
    eImage,
    eImageSource,
    eCurve,
    eNode,
  };

  enum class Event
  {
    eValueModified,
    eNodeDeleted
  };

  using handle = dshandle;

  DataSource() = default;
  DataSource(handle n) : self(n) {}
  virtual ~DataSource()
  {
    propagate(Event::eNodeDeleted);
    self = {};
  }

  // Option
  virtual bool isEnabled(Pipeline const&) = 0;
  // Ensure data exists in consumable form (ex. In the GPU)
  virtual bool       ensure(Pipeline&) = 0;
  virtual Type       getType() const         = 0;
  virtual DataFormat getFormat() const       = 0;

  // Accept the change event from a source, for a data source that is dependent
  virtual void accept(dshandle source, Event) = 0;

  virtual bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
  {
    if (!getFromDataStream(dataStream, serialIdx, self.reserved))
      return false;
    return fromDataStream(dataStream, serialIdx);
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

  void propagate(Event);
  void onParamChange(dshandle oldValue, dshandle newValue);
  bool setParamSource(uint32_t paramIdx, dshandle);

  static bool isValid(dshandle);

  using exchange = std::pair<dshandle, bool>;

  virtual void fillDescriptor(Pipeline const&, GfxDescriptorSet::rhandle&, std::byte*) = 0;

  static inline bool isWithinTile(ivec2 tile, ivec2 tileConstraintMin, ivec2 tileConstraintMax) 
  {
    return ((tileConstraintMin[0] < 0 || tileConstraintMax[0] < 0) ||
            (tileConstraintMin[0] >= tile[0] && tile[0] < tileConstraintMax[0])) &&
           ((tileConstraintMin[1] < 0 || tileConstraintMax[1] < 0) ||
            (tileConstraintMin[1] >= tile[1] && tile[1] < tileConstraintMax[1]));
  }


protected:
  virtual bool     fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx);
  virtual void     toDataStreamImpl(std::vector<uint8_t>& dataStream) const;
  virtual exchange setParamSourceImpl(uint32_t paramIdx, dshandle) = 0;

  dshandle self;
};

} // namespace terra