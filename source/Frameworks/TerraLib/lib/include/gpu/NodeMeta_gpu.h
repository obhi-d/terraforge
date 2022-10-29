#pragma once

#include "NodeMeta.h"

namespace terra
{

class NodeMeta_gpu : public NodeMeta
{
  using Run = void (*)(Node& node, Pipeline&);

  Run run   = nullptr;
};

} // namespace terra