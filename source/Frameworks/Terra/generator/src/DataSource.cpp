
#include "DataSource.h"
#include "Terra.h"

namespace terra
{

void DataSource::propagate(Event ev)
{
  forEachDependent(
    [ev, self = this->self](Source d)
    {
      if (DataSource::isValid(d.source))
      {
        get().get<DataSource>(d.source).accept(self, ev);
        // Even if node is deleted we just want to inform that value has
        // been modified to decendants
        get().get<DataSource>(d.source).propagate(Event::eValueModified);
      }
    });
}

void DataSource::onParamChange(uint32_t i, Source oldValue, Source newValue)
{
  if (oldValue.source && get().isValid(oldValue.source))
  {
    auto& oldData = get().get<DataSource>(oldValue.source);
    oldData.remove(self);
  }
  if (newValue.source && get().isValid(newValue.source))
  {
    auto& newData = get().get<DataSource>(newValue.source);
    newData.add(self);
  }
  onParamChangeImpl(i, oldValue, newValue);
  updateVersion();
}

bool DataSource::setParamSource(uint32_t paramIdx, Source value)
{
  auto [old, accept] = setParamSourceImpl(paramIdx, value);
  if (accept)
  {
    onParamChange(paramIdx, old, value.source);
    return true;
  }
  return false;
}

bool DataSource::isValid(HDataSource ds)
{
  return get().isValid(ds);
}

bool DataSource::isNode(HDataSource ds)
{
  return isValid(ds) && get().get<DataSource>(ds).getType() == Type::eNode;
}

void DataSource::accept(Source source, Event ev)
{
  if (ev == Event::eValueModified || ev == Event::eNodeDeleted)
    version++;
}

void DataSource::prepareGeneration(HDataSource ds, Pipeline& pipe)
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->prepareGeneration(pipe);
}

void DataSource::beginIteration(HDataSource ds, Pipeline& pipe)
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->beginIteration(pipe);
}

void DataSource::endIteration(HDataSource ds, Pipeline& pipe)
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->endIteration(pipe);
}

bool DataSource::isPushable(HDataSource ds, uvec2 tile, uvec2 tileConstraintOffset, uvec2 tileConstraintSize)
{
  return DataSource::isValid(ds) && DataSource::isWithinTile(tile, tileConstraintOffset, tileConstraintSize) &&
         get().get<DataSource>(ds).isPushable();
}

} // namespace terra