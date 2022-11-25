
#pragma once

#include "Node.h"
#include "hyb/HybridNodeMeta.h"
#include <unordered_map>

namespace terra
{
class SourceBuilder;
/// @brief Basics of node
/// A node can be executed on:
///   - CPU  : Fully executes and genertes results
///   - GPU  : Injects code and parameters into Source generator
///     - GPU nodes can generate results when required
///   - GPUE : Always executes and generates results
struct HybridNode : public Node
{
  enum class Queue
  {
    eCPU,
    eCompute,
    eGraphics
  };

  virtual bool  needsPipelineExecute() const                     = 0;
  virtual bool  isInjectSupported() const                        = 0;
  virtual Queue getQueue() const                                 = 0;
  virtual void  execute(HybridPipeline&) const                   = 0;
  virtual void  copyToCPU()                                      = 0;
  virtual bool  buildIntoSource(SourceBuilder&, HybridPipeline&) = 0;
  virtual bool  fillParameters(SourceBuilder&, HybridPipeline&)  = 0;
};

struct ClassicHybridNode : public HybridNode
{
  virtual bool needsPipelineExecute() const
  {
    return true;
  }
  virtual bool isInjectSupported() const
  {
    return false;
  }
  virtual Queue getQueue() const
  {
    return Queue::eGraphics;
  }
  virtual bool buildIntoSource(SourceBuilder&, HybridPipeline&)
  {
    return false;
  }
  virtual void execute(HybridPipeline&) const                  = 0;
  virtual void copyToCPU()                                     = 0;
  virtual bool fillParameters(SourceBuilder&, HybridPipeline&) = 0;
};
} // namespace terra