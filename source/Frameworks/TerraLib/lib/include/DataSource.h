
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
  ScalarValue constVal;
  DataSource() = default;
  DataSource(hnode n) : node(n) {}

  inline bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
  {
    if (!getFromDataStream(dataStream, serialIdx, node))
      return false;
    if (!getFromDataStream(dataStream, serialIdx, constVal.ivalue2))
      return false;
    return true;
  }

  inline void toDataStream(std::vector<uint8_t>& dataStream) const
  {
    addToDataStream(dataStream, node.reserved);
    addToDataStream(dataStream, constVal.ivalue2);
  }
};
} // namespace terra