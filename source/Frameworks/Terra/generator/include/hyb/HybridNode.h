
#pragma once

#include "HashMachine.h"
#include "Node.h"
#include "ShaderOptions.h"
#include "ShaderProgram.h"
#include "hyb/HybridBuffer.h"
#include "hyb/HybridNodeMeta.h"
#include <acl/dynamic_array.hpp>
#include <unordered_map>

namespace terra
{

class HybridPipeline;
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

  enum class Result
  {
    eDone,
    eContinue,
    eFailed,
    eWaiting
  };

  virtual bool   needsPipelineExecute() const        = 0;
  virtual bool   isSourceModifier() const            = 0;
  virtual Queue  getQueue() const                    = 0;
  virtual Result execute(HybridPipeline&) const      = 0;
  virtual void   prepare(HybridPipeline&)            = 0;
  virtual void   probe(HybridPipeline&, ProgramKey&, HashMachine&) = 0;
};

struct ClassicHybridNode : public HybridNode
{
  bool needsPipelineExecute() const override
  {
    return true;
  }
  bool isSourceModifier() const override
  {
    return false;
  }
  Queue getQueue() const override
  {
    return Queue::eGraphics;
  }
  Result execute(HybridPipeline&) const override
  {
    return Result::eDone;
  }
  void modifyOption(ShaderOptions&) const {}

  uvec2 constraintTileStart = uvec2(0, 0);
  uvec2 constraintTileCount = uvec2(0, 0);
};

struct GpuNode : public ClassicHybridNode
{
  struct Data
  {
    ProgramKey                               key;
    ShaderProgramRef                         program;
    acl::dynamic_array<HybridBuffer::handle> inputs;
    acl::dynamic_array<HybridBuffer::handle> outputs;
  };

  void   modifyOption(ShaderOptions&) const;
  void   prepare(HybridPipeline&) override;
  void   probe(HybridPipeline&, ProgramKey&, HashMachine&) override;
  Result execute(HybridPipeline&) const;
  bool   fillParameters(ShaderProgram&, HybridPipeline&);

  std::vector<Data> nodeData;
};
} // namespace terra