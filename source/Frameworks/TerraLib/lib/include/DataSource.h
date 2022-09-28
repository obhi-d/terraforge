
#pragma once

#include "GpuBuffer.h"
namespace terra
{
class Node;
using NodePtr = std::shared_ptr<Node>;
class DataSource
{
  NodePtr source       = nullptr;
  float   defaultValue = 0.0f;
};
} // namespace terra