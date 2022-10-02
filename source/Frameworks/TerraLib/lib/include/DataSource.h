
#pragma once

#include "Serializer.h"
#include "Common.h"

namespace terra
{
class Node;
class DataSource
{
public:
  hnode node;
  float       constValue = 0.0f;

  DataSource() = default;
  DataSource(hnode n) : node(n) {}

  inline bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
  {
    if (!getFromDataStream(dataStream, serialIdx, node))
      return false;
    if (!getFromDataStream(dataStream, serialIdx, constValue))
      return false;
    return true;
  }

  inline void toDataStream(std::vector<uint8_t>& dataStream) const
  {
    addToDataStream(dataStream, node.reserved);
    addToDataStream(dataStream, constValue);
  }
};
} // namespace terra