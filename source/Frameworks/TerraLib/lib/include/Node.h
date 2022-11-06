#pragma once
#include "NodeMeta.h"
#include <acl/dynamic_array.hpp>

namespace terra
{

class Node : public DataSource
{
public:

  acl::dynamic_array<Parameter> parameters;

  std::u8string   name;
  NodeMeta const& meta;

  Node(NodeMeta const& m);
  
  uint32 getNumParams() const
  {
    return (uint32)meta.parameterDef.size();
  }

  Parameter const& param(uint32_t i) const
  {
    return parameters[i];
  }

  void state(uint32_t i, ScalarValue sv) 
  {
    parameters[i] = sv;
  }

  Parameter param(uint32_t i, Parameter&& sv)
  {
    auto old = parameters[i];
    parameters[i] = std::move(sv);
    dshandle oldSrc, newSrc;
    if (std::holds_alternative<Source>(old))
      oldSrc = std::get<Source>(old).source;
    if (std::holds_alternative<Source>(sv))
      newSrc = std::get<Source>(sv).source;
    onParamChange(oldSrc, newSrc);
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

  virtual DataFormat getFormat() const
  {
    return DataFormat(DataType::eBuffer);
  }

  void accept(dshandle source, Event) override;
};

}