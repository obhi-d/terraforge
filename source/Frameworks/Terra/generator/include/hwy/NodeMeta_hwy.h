
#pragma once
#include <NodeMeta.h>

namespace terra
{

class Pipeline_hwy;

class NodeMeta_hwy : public NodeMeta
{
public:
  inline NodeMeta_hwy() = default;
  inline NodeMeta_hwy(NoDomain) : NodeMeta(NoDomain{}) {}

  using Fn  = void (*)(Node& node, Pipeline_hwy&, uint32_t thread);
  using Pfn = void (*)(Node& node, Pipeline_hwy&);

  Pfn prepare = nullptr;
  Pfn beginIt = nullptr;
  Pfn endIt   = nullptr;
  Fn  fn      = nullptr;

  void prepareGeneration(Node&, Pipeline&) const final;
  void beginIteration(Node&, Pipeline&) const final;
  void endIteration(Node&, Pipeline&) const final;

  static void domain(Parameter const& param, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void run(HDataSource, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void end(HDataSource, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void fill(float, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void write(Parameter const&, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
};

} // namespace terra