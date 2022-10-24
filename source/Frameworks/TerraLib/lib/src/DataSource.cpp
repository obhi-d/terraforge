
#include "Terra.h"
#include "DataSource.h"

namespace terra
{

void DataSource::propagate(Event ev) 
{
  forEachDependent([ev, self = this->self](dshandle d) 
    {
      get().get<DataSource>(d).accept(self, ev);
      // Even if node is deleted we just want to inform that value has
      // been modified to decendants
      get().get<DataSource>(d).propagate(Event::eValueModified);
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
  propagate(Event::eValueModified);
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

}