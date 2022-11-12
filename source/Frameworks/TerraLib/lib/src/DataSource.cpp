
#include "Terra.h"
#include "DataSource.h"

namespace terra
{

void DataSource::propagate(Event ev) 
{
  forEachDependent([ev, self = this->self](dshandle d) 
    {
      if (DataSource::isValid(d))
      {
        get().get<DataSource>(d).accept(self, ev);
        // Even if node is deleted we just want to inform that value has
        // been modified to decendants
        get().get<DataSource>(d).propagate(Event::eValueModified);
      }
    });
}

void DataSource::onParamChange(dshandle oldValue, dshandle newValue) 
{
  if (oldValue && get().isValid(oldValue))
  {
    auto& oldData = get().get<DataSource>(oldValue);
    oldData.remove(self);
  }
  if (newValue && get().isValid(newValue))
  {
    auto& newData = get().get<DataSource>(newValue);
    newData.add(self);
  }
  updateVersion();
}

bool DataSource::setParamSource(uint32_t paramIdx, Source value)
{
  auto [old, accept] = setParamSourceImpl(paramIdx, value);
  if (accept)
  {
    onParamChange(old, value.source);
    return true;
  }
  return false;
}

bool DataSource::isValid(dshandle ds) 
{
  return get().isValid(ds);
}

bool DataSource::isNode(dshandle ds)
{
  return isValid(ds) && get().get<DataSource>(ds).getType() == Type::eNode;
}

void DataSource::accept(dshandle source, Event ev) 
{
  if (ev == Event::eValueModified || ev == Event::eNodeDeleted)
    version++;
}

void DataSource::prepareGeneration(dshandle ds, Pipeline& pipe) 
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->prepareGeneration(pipe);
}

void DataSource::beginIteration(dshandle ds, Pipeline& pipe)
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->beginIteration(pipe);
}

void DataSource::endIteration(dshandle ds, Pipeline& pipe)
{
  auto p = get().getIf<DataSource>(ds);
  if (p)
    p->endIteration(pipe);
}

}