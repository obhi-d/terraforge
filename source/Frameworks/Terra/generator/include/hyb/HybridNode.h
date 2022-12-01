
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
class GpuProgramBuilder;
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

  virtual std::string_view getFunction() const                                                  = 0;
  virtual bool             needsPipelineExecute() const                                         = 0;
  virtual bool             isSourceModifier() const                                             = 0;
  virtual Queue            getQueue() const                                                     = 0;
  virtual Result           execute(HybridPipeline&) const                                       = 0;
  virtual bool             prepare(HybridPipeline&)                                             = 0;
  virtual void             probe(HybridPipeline&, ProgramKey&, HashMachine&)                    = 0;
  virtual void             build(HybridPipeline&, GpuProgramBuilder&)                           = 0;
  virtual void             execute(HybridPipeline&, ShaderProgramInstance&, uint32_t idx) const = 0;
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
  std::string_view getFunction() const override
  {
    return {};
  }
  void  build(HybridPipeline&, GpuProgramBuilder&) override {}
  void  execute(HybridPipeline&, ShaderProgramInstance&, uint32_t) const override {}
  uvec2 constraintTileStart = uvec2(0, 0);
  uvec2 constraintTileCount = uvec2(0, 0);
};

struct GpuNode : public ClassicHybridNode
{
  struct Data
  {
    ProgramKey                               key;
    ShaderProgramRef                         program;
    acl::dynamic_array<HybridBuffer::handle> outputs;
    uint64_t                                 injectMask = 0;
  };

  std::string_view getFunction() const override;
  bool             prepare(HybridPipeline&) override;
  void             probe(HybridPipeline&, ProgramKey&, HashMachine&) override;
  void             build(HybridPipeline&, GpuProgramBuilder&) override;
  Result           execute(HybridPipeline&) const;
  void             execute(HybridPipeline&, ShaderProgramInstance&, uint32_t idx) const override;
  virtual void     pushOutputs(HybridPipeline&, ShaderProgramInstance&) const;

  std::vector<Data> nodeData;
};
} // namespace terra