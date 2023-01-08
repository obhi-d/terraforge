
#include "Node.h"

namespace terra
{
Node::Node(NodeMeta const& m) : meta(m), name(m.displayInfo.name) {}

void Node::accept(Source source, Event ev)
{
  if (ev == Event::eNodeDeleted)
  {
    for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
    {
      auto p = param(i);
      if (std::holds_alternative<Source>(p))
      {
        if (std::get<Source>(p) == source)
          param(i, meta.parameterDef[i].getDefault());
      }
    }
  }
  DataSource::accept(source, ev);
}

void Node::getSourcesImpl(SourceSet& sources) const
{
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto p = param(i);
    if (std::holds_alternative<Source>(p))
    {
      if (DataSource::isValid(std::get<Source>(p).source))
        sources.emplace(std::get<Source>(p).source);
    }
  }
}

void Node::prepareGeneration(Pipeline& pipe)
{
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto p = param(i);
    if (std::holds_alternative<Source>(p))
    {
      DataSource::prepareGeneration(std::get<Source>(p).source, pipe);
    }
  }
}

void Node::beginIteration(Pipeline& pipe)
{
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto p = param(i);
    if (std::holds_alternative<Source>(p))
    {
      DataSource::beginIteration(std::get<Source>(p).source, pipe);
    }
  }
}

void Node::endIteration(Pipeline& pipe)
{
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto p = param(i);
    if (std::holds_alternative<Source>(p))
    {
      DataSource::endIteration(std::get<Source>(p).source, pipe);
    }
  }
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

void Node::updateAutos(uint32_t i)
{
  constexpr uint32_t one = 1;
  forEachBit(
    [&](auto p)
    {
      auto idx = (uint32_t)meta.parameterDef[p].format.semantic;
      if (idx < meta.autoRegistry.size() && meta.autoRegistry[idx].change)
      {
        meta.autoRegistry[idx].change(*this, p);
      }
      return true;
    },
    meta.parameterDef[i].dependencies);
}

} // namespace terra