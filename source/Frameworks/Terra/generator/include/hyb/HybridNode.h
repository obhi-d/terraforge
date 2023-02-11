
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
    ePause,
    eSkip,
    eWaiting
  };

  HybridNode(NodeMeta const& m) : Node(m) {}

  virtual Queue    getQueue() const                                                                 = 0;
  virtual Result   execute(HybridPipeline&)                                                         = 0;
  virtual Result   preExecute(HybridPipeline&, std::vector<Parameter>&)                             = 0;
  virtual void     executeImpl(HybridPipeline&, std::vector<Parameter>&)                            = 0;
  virtual Result   postExecute(HybridPipeline&, Result preState)                                    = 0;
  virtual bool     prepare(HybridPipeline&)                                                         = 0;
  virtual void     probe(HybridPipeline&, ProgramKey&, HashMachine&)                                = 0;
  virtual void     push(HybridPipeline&, ShaderProgramInstance&, uint32_t outIdx, DataFormat inFmt) = 0;
  virtual uint32_t getComputeVersion() const                                                        = 0;
  virtual void     setComputeVersion(uint32_t)                                                      = 0;
};

struct ClassicHybridNode : public HybridNode
{
  ClassicHybridNode(NodeMeta const& m) : HybridNode(m) {}

  Queue getQueue() const override
  {
    return Queue::eGraphics;
  }

  Result execute(HybridPipeline& pipe) override
  {
    std::vector<Parameter> autoParams;
    Result                 pre = preExecute(pipe, autoParams);
    if (pre != Result::eFailed)
    {
      if (pre != Result::ePause && pre != Result::eSkip)
        executeImpl(pipe, autoParams);
      return postExecute(pipe, pre);
    }
    return pre;
  }
  void   push(HybridPipeline&, ShaderProgramInstance&, uint32_t outIdx, DataFormat inFmt) override {}
  Result preExecute(HybridPipeline&, std::vector<Parameter>&) override;
  void   executeImpl(HybridPipeline&, std::vector<Parameter>&) override {}
  Result postExecute(HybridPipeline&, Result preState) override;

  virtual uint32_t getComputeVersion() const
  {
    return computeVer;
  }

  virtual void setComputeVersion(uint32_t v)
  {
    computeVer = v;
  }

  uvec2    constraintTileStart = uvec2(0, 0);
  uvec2    constraintTileCount = uvec2(0, 0);
  uint32_t computeVer          = 1440;
};

struct GpuNode : public ClassicHybridNode
{
  struct Data
  {
    ProgramKey                        key;
    ShaderOptions                     activeOptions;
    GpuPipelinePtr                    gpuPasses;
    std::vector<HybridBuffer::handle> outputs;
    uint64_t                          injectMask = 0;
  };

  GpuNode(NodeMeta const& m) : ClassicHybridNode(m) {}

  bool   prepare(HybridPipeline&) override;
  void   probe(HybridPipeline&, ProgramKey&, HashMachine&) override;
  void   build(HybridPipeline&, uint32_t pass, SourceBuilder&);
  void   executeImpl(HybridPipeline&, std::vector<Parameter>&) override;
  void   push(HybridPipeline&, ShaderProgramInstance&, uint32_t outIdx, DataFormat inFmt) override;
  Result postExecute(HybridPipeline&, Result preState) override;

  virtual void createResources(GpuNode::Data&, HybridPipeline&, GpuNodeMeta const&);
  virtual void pushOutputs(HybridPipeline&, uint32_t pass, ShaderProgramInstance&);

  std::vector<Data> nodeData;
};

struct GpuScriptNode : public GpuNode
{

  std::vector<Parameter> parameters;

  static bool isSourceType(DataTypeEnum ty)
  {
    return ty == DataTypeEnum::eCurveData || ty == DataTypeEnum::eImage || ty == DataTypeEnum::eInput ||
           ty == DataTypeEnum::ePostProcess || ty == DataTypeEnum::eBuffer;
  }

  void      set(uint32_t i, Parameter const&);
  Parameter get(uint32_t i) const;

  GpuScriptNode(NodeMeta const& m);
};

struct GpuImageNode : public GpuNode
{
  HDataSource image;
  glm::vec2   sampleOffset = vec2(0.f, 0.f);
  glm::vec2   sampleScale  = vec2(1.f, 1.f);
  float       scale        = 1.0f;
  uint32_t    version      = 0xffffffff;

  GpuImageNode(NodeMeta const& m);
  ~GpuImageNode();

  void selfUpdated() override;
  void executeImpl(HybridPipeline&, std::vector<Parameter>&) override;
};

struct GpuCurveNode : public GpuNode
{
  HDataSource curve;
  glm::vec2   scale;
  uint32_t    version = 0xffffffff;

  GpuCurveNode(NodeMeta const& m);
  ~GpuCurveNode();

  void selfUpdated() override;
  void executeImpl(HybridPipeline&, std::vector<Parameter>&) override;
};

} // namespace terra