
#pragma once
#include <NodeMeta.h>

namespace terra
{

class Pipeline_hwy;

class NodeMeta_hwy : public NodeMeta
{
public:
  using Fn = void (*)(Node& node, Pipeline_hwy&, uint32_t thread);

  Fn fn = nullptr;
    
  static void run(dshandle, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void end(dshandle, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void fill(float, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
  static void write(Parameter const&, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes);
};

} // namespace terra