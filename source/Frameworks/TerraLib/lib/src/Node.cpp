
#include "Node.h"

namespace terra
{
Node::Node(NodeMeta const& m) : meta(m), name(m.displayInfo.name), parameters((uint32_t)m.parameterDef.size(), ScalarValue{})
{
  for (uint32_t i = 0; i < parameters.size(); ++i)
  {
    parameters[i] = m.parameterDef[i].getDefault();
  }
}

void Node::accept(dshandle source, Event ev) 
{
  if (ev == Event::eNodeDeleted)
  {
    for (uint32_t i = 0; i < parameters.size(); ++i)
    {
      auto& p = parameters[i];
      if (std::holds_alternative<Source>(p))
      {
        if (std::get<Source>(p).source == source)
          p = meta.parameterDef[i].getDefault();
      }
    }
  }
  DataSource::accept(source, ev);
}

HelpInfo Node::getHelpInfo(HelpType type, int param) const
{
  switch (type)
  {
  case HelpType::eOutput:
  case HelpType::eDataSource:
    return meta.displayInfo;
  case HelpType::eParameter:
    if (param < (int)meta.parameterDef.size() && param >= 0)
      return meta.parameterDef[param].displayInfo;
  }
  return {};
}

}