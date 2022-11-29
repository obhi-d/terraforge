#pragma once

#include "NodeMeta.h"
#include "ShaderOptions.h"
#include "hyb/ShaderProgram.h"

namespace terra
{

class HybridPipeline;
class HybridNodeMeta : public NodeMeta
{};

struct ProgramKey
{
  uint64_t                   hash       = 0;
  uint32_t                   active     = 0;
  uint32_t                   probeCount = 0;
  std::vector<ShaderOptions> options;

  inline bool operator==(ProgramKey const&) const noexcept = default;
  inline bool operator!=(ProgramKey const&) const noexcept = default;

  struct hasher
  {
    inline uint32_t operator()(ProgramKey const& option) const noexcept
    {
      return option.hash;
    }
  };
};

class GpuNodeMeta : public HybridNodeMeta
{
public:
  ShaderProgramPtr findProgram(ProgramKey const& key) const;
  void             addProgram(ProgramKey const& key, ShaderProgramPtr program) const;

  uint32 getDictionaryIdx() const
  {
    return dictionaryIdx;
  }

protected:
  void prepare() override;

  using ShaderMap = std::unordered_map<ProgramKey, ShaderProgramPtr, ProgramKey::hasher>;
  static std::unordered_map<uint32_t, ShaderMap> shaderMaps;

  uint32_t dictionaryIdx = 0xffffffff;
};

} // namespace terra