
#include "hyb/HybridNode.h"
#include "Terra.h"
#include "hyb/HybridNodeMeta.h"
#include "hyb/HybridPipeline.h"

namespace terra
{

void GpuNode::prepare(HybridPipeline& pipe)
{
  pipe.push(self);
  if (pipe.getId() >= nodeData.size())
    nodeData.resize(pipe.getId() + 1);
  auto&       ndat    = nodeData[pipe.getId()];
  auto        program = ndat.program.lock();
  HashMachine machine{0};
  ProgramKey  option;
  probe(pipe, option, machine);
  option.hash         = machine.value;
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  if (!program || ndat.key != option)
    program = gpuMeta.findProgram(option);
  if (!program)
  {
    GpuProgramBuilder builder;
    for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
    {
      auto const& def = meta.parameterDef[i];
      switch (def.format.type)
      {
      case DataType::eBuffer:
        builder.bind_buffer()
        
        break;
      case DataType::eInt2:
      }
    }
    // use GpuProgramBuilder to build a program
  }
}

void GpuNode::probe(HybridPipeline& pipe, ProgramKey& option, HashMachine& machine)
{
  uint32_t      optionIdx = 0;
  ShaderOptions shoption;
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto const& def = meta.parameterDef[i];
    auto        p   = param(i);
    if (std::holds_alternative<Source>(p))
    {
      auto source = std::get<Source>(p).source;
      if (DataSource::isValid(source) &&
          DataSource::isWithinTile(pipe.getTileId(), constraintTileStart, constraintTileCount))
      {
        //

        auto& node = get().get<HybridNode>(source);
        if (node.isSourceModifier())
        {
          node.probe(pipe, option);
          option.probeCount++;
        }
        node.prepare(pipe);
        option.active++;
        shoption.setOption(optionIdx);
        optionIdx++;
      }
    }
    else
    {
      switch (def.format.type)
      {
      case DataType::eCurveData:
      case DataType::eImage:
      case DataType::eInput:
      case DataType::ePostProcess:
      case DataType::eBuffer:
        optionIdx++;
        break;
      case DataType::eBool:
        if (std::get<ScalarValue>(p).bvalue)
          shoption.setOption(optionIdx);
        optionIdx++;
        break;
      case DataType::eEnum:
        shoption.setOption(optionIdx + std::get<ScalarValue>(p).ivalue);
        optionIdx += def.maxEnum;
        break;
      }
    }
  }

  machine(shoption.options.mask);
  option.options.emplace_back(shoption);
}

HybridNode::Result ShaderNode::execute(HybridPipeline& pipe) const
{
  // check if any conditions have changed
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto p = param(i);
    if (std::holds_alternative<Source>(p))
    {
      if (DataSource::isValid(std::get<Source>(p).source))
        get().get<HybridNode>(std::get<Source>(p).source).prepare(pipe);
    }
  }
}

} // namespace terra