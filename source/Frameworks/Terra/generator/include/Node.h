#pragma once
#include "NodeMeta.h"
#include <acl/dynamic_array.hpp>

namespace terra
{

class Node : public DataSource
{
public:
  Source          domain;
  std::u8string   name;
  NodeMeta const& meta;

  uint32 getNumParams() const
  {
    return (uint32)meta.parameterDef.size();
  }

  Parameter param(uint32_t i) const
  {
    return meta.parameterDef[i].getter(*this);
  }

  void state(uint32_t i, ScalarValue sv)
  {
    meta.parameterDef[i].setter(*this, sv);
  }

  Parameter param(uint32_t i, Parameter sv)
  {
    auto old = meta.parameterDef[i].getter(*this);
    meta.parameterDef[i].setter(*this, sv);
    HDataSource oldSrc, newSrc;
    if (std::holds_alternative<Source>(old))
      oldSrc = std::get<Source>(old).source;
    if (std::holds_alternative<Source>(sv))
      newSrc = std::get<Source>(sv).source;
    onParamChange(i, oldSrc, newSrc);
    return old;
  }

  Parameter resetValue(uint32_t i)
  {
    return param(i, meta.parameterDef[i].getDefault());
  }

  virtual Type getType() const
  {
    return Type::eNode;
  }

  DataFormat getFormat(uint32_t i) const override
  {
    return meta.outputs[i];
  }

  void getSourcesImpl(SourceSet&) const final;
  void prepareGeneration(Pipeline&) final;
  void beginIteration(Pipeline&) final;
  void endIteration(Pipeline&) final;

  void     accept(Source source, Event) override;
  HelpInfo getHelpInfo(HelpType type, int param = -1) const final;

  Node(NodeMeta const& m);
};

} // namespace terra