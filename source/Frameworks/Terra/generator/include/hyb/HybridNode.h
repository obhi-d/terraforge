
#pragma once

#include "HashMachine.h"
#include "Node.h"
#include "ShaderOptions.h"
#include "hyb/HybridBuffer.h"
#include "hyb/HybridNodeMeta.h"
#include "hyb/ShaderProgramInstance.h"
#include <acl/dynamic_array.hpp>
#include <unordered_map>

namespace terra
{
struct SourceBuilder;
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

  virtual Queue  getQueue() const                                                                  = 0;
  virtual Result execute(HybridPipeline&)                                                          = 0;
  virtual bool   preExecute(HybridPipeline&)                                                       = 0;
  virtual void   executeImpl(HybridPipeline&)                                                      = 0;
  virtual Result postExecute(HybridPipeline&)                                                      = 0;
  virtual bool   prepare(HybridPipeline&)                                                          = 0;
  virtual void   probe(HybridPipeline&, ProgramKey&, HashMachine&)                                 = 0;
  virtual void   push(HybridPipeline&, ShaderProgramInstance&, uint32_t paramIdx, uint32_t outIdx) = 0;
};

struct ClassicHybridNode : public HybridNode
{
  Queue getQueue() const override
  {
    return Queue::eGraphics;
  }
  Result execute(HybridPipeline& pipe) override
  {
    if (preExecute(pipe))
    {
      executeImpl(pipe);
      return postExecute(pipe);
    }
    return Result::eFailed;
  }
  void   push(HybridPipeline&, ShaderProgramInstance&, uint32_t paramIdx, uint32_t outIdx) override {}
  bool   preExecute(HybridPipeline&) override;
  void   executeImpl(HybridPipeline&) override {}
  Result postExecute(HybridPipeline&) override;

  uvec2 constraintTileStart = uvec2(0, 0);
  uvec2 constraintTileCount = uvec2(0, 0);
};

struct GpuNode : public ClassicHybridNode
{
  struct Data
  {
    ProgramKey                        key;
    ShaderOptions                     activeOptions;
    GpuPipelineRef                    gpuPasses;
    std::vector<HybridBuffer::handle> outputs;
    uint64_t                          injectMask = 0;
  };

  bool   isSourceModifier() const;
  bool   prepare(HybridPipeline&) override;
  void   probe(HybridPipeline&, ProgramKey&, HashMachine&) override;
  void   build(HybridPipeline&, uint32_t pass, SourceBuilder&);
  void   executeImpl(HybridPipeline&) override;
  void   push(HybridPipeline&, ShaderProgramInstance&, uint32_t paramIdx, uint32_t outIdx) override;
  Result postExecute(HybridPipeline&) override;

  virtual void pushOutputs(HybridPipeline&, uint32_t pass, ShaderProgramInstance&);

  std::vector<Data> nodeData;
};

struct GpuScriptNode : public GpuNode
{
  std::vector<Parameter> parameters;
};
} // namespace terra