#pragma once

#include "NodeMeta.h"
#include "ShaderOptions.h"
#include "hyb/ShaderProgramInstance.h"

namespace terra
{

struct HybridNode;
class HybridPipeline;

class HybridNodeMeta : public NodeMeta
{};

struct ProgramKey
{
  uint64_t      hash       = 0;
  uint64_t      probeMask  = 0;
  uint32_t      probeCount = 0;
  uint32_t      active     = 0;
  ShaderOptions options;

  inline bool operator==(ProgramKey const&) const noexcept = default;
  inline bool operator!=(ProgramKey const&) const noexcept = default;

  struct hasher
  {
    inline uint64_t operator()(ProgramKey const& option) const noexcept
    {
      return option.hash;
    }
  };
};

class GpuNodeMeta : public HybridNodeMeta
{
public:
  static constexpr uint32_t kOutputMask = 0x80000000;
  struct GpuPass
  {
    std::string           function;
    std::string           extensions;
    std::string           shaderContent;
    std::vector<uint32_t> parameters;
    std::vector<uint32_t> outputs;
  };

  GpuPipelinePtr findProgram(ProgramKey const& key) const;
  void           addProgram(ProgramKey const& key, GpuPipelinePtr program) const;

  uint32 getDictionaryIdx() const
  {
    return dictionaryIdx;
  }

  uint32_t getNumPasses() const
  {
    return (uint32_t)passes.size();
  }

  GpuPass const& getCode(uint32_t pass) const
  {
    return passes[pass];
  }

  void prepare() override;

  static void registerKnownMeta();

  using GpuPipelineMap = std::unordered_map<ProgramKey, GpuPipelineRef, ProgramKey::hasher>;
  static GpuPipelineMap shaderMaps;

  std::vector<GpuPass> passes;
  uint32_t             dictionaryIdx = 0xffffffff;
};

class GpuScriptNodeMeta : public GpuNodeMeta
{};
} // namespace terra