
#include "hyb/HybridNode.h"
#include "Logger.h"
#include "Terra.h"
#include "hyb/HybridNodeMeta.h"
#include "hyb/HybridPipeline.h"

namespace terra
{

bool GpuNode::prepare(HybridPipeline& pipe)
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
    auto builder = GpuProgramBuilder(meta.id);
    build(pipe, builder);
    // use GpuProgramBuilder to build a program
    program = builder.finalize();
    if (!program)
    {
      logError("Failed to compile program: {}", gpuMeta.getCode().function);
      return false;
    }
    gpuMeta.addProgram(option, program);
    ndat.outputs = acl::dynamic_array<HybridBuffer::handle>(program->outputCount);
    for (auto& o : ndat.outputs)
      o = pipe.declareBuffer();
  }

  if (program)
    program->touch();

  ndat.program = std::move(program);
  ndat.key     = option;

  return true;
}

void GpuNode::build(HybridPipeline& pipe, GpuProgramBuilder& builder)
{
  auto&         ndat      = nodeData[pipe.getId()];
  auto const&   gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t      optionIdx = 0;
  ShaderOptions shoption{gpuMeta.getDictionaryIdx()};

  enum class Action : uint8_t
  {
    eComputeNode,
    ePushConstant,
    eSampleNode
  };

  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto const& def      = meta.parameterDef[i];
    auto        p        = param(i);
    Action      paramAct = Action::eSampleNode;
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
          auto oldid = builder.swap_id(optionIdx + 1);
          node.build(pipe, builder);
          builder.swap_id(oldid);
          paramAct = Action::eComputeNode;
        }
        shoption.setOption(optionIdx);
      }
      else
        paramAct = Action::ePushConstant;
    }

    switch (def.format.type)
    {
    case DataType::eCurveData:
    case DataType::eImage:
    case DataType::eInput:
    case DataType::ePostProcess:
    case DataType::eBuffer:
      switch (paramAct)
      {
      case Action::eSampleNode:
        builder.sample_param(def.name, def.format.type, def.format.scalarSubType, optionIdx);
        break;
      case Action::eComputeNode:
        builder.compute_param(def.name, def.format.type, def.format.scalarSubType, optionIdx);
        break;
      case Action::ePushConstant:
        builder.set_param(def.name, def.format.scalarSubType);
        break;
      }
      optionIdx++;
      break;
    case DataType::eFloat:
    case DataType::eFloat2:
    case DataType::eInt:
    case DataType::eInt2:
      builder.set_param(def.name, def.format.type);
      break;
    case DataType::eBool:
      if (std::get<ScalarValue>(p).bvalue)
        shoption.setOption(optionIdx);
      optionIdx++;
      break;
    case DataType::eEnum:
      shoption.setOption(optionIdx + (uint32_t)std::get<ScalarValue>(p).ivalue);
      optionIdx += def.maxEnum;
      break;
    }
  }
  for (auto& o : meta.outputs)
    builder.bind_output_texture(o.name);
  builder.push_extension(gpuMeta.getCode().extensions);
  builder.append_code(gpuMeta.getCode().shaderContent);
}

std::string_view GpuNode::getFunction() const
{
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  return gpuMeta.getCode().function;
}

void GpuNode::probe(HybridPipeline& pipe, ProgramKey& option, HashMachine& machine)
{
  auto const&   gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t      optionIdx = 0;
  ShaderOptions shoption{gpuMeta.getDictionaryIdx()};
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
          node.probe(pipe, option, machine);
          option.probeMask |= 1ull << i;
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
        shoption.setOption(optionIdx + (uint32_t)std::get<ScalarValue>(p).ivalue);
        optionIdx += def.maxEnum;
        break;
      }
    }
  }

  machine(shoption.options.mask);
  option.options = shoption;
}

HybridNode::Result GpuNode::execute(HybridPipeline& pipe) const
{
  auto&       ndat      = nodeData[pipe.getId()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  GpuBuffer&  ubo       = pipe.getUBO();
  auto        program   = ShaderProgramInstance(ndat.program.lock());
  uint32_t    optionIdx = 0;
  // check if any conditions have changed
  auto pushScalar = [&program](Parameter const& p, DataType type)
  {
    switch (type)
    {
    case DataType::eFloat:
      program.pushValue(std::get<ScalarValue>(p).value);
      break;
    case DataType::eFloat2:
      program.pushValue(std::get<ScalarValue>(p).value2);
      break;
    case DataType::eInt:
      program.pushValue(std::get<ScalarValue>(p).ivalue);
      break;
    case DataType::eInt2:
      program.pushValue(std::get<ScalarValue>(p).ivalue2);
      break;
    }
  };
  for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
  {
    auto const& def = meta.parameterDef[i];
    auto        p   = param(i);

    switch (def.format.type)
    {
    case DataType::eCurveData:
    case DataType::eImage:
    case DataType::eInput:
    case DataType::ePostProcess:
    case DataType::eBuffer:
      if (ndat.key.options.isSet(optionIdx))
      {
        auto  source = std::get<Source>(p).source;
        auto& node   = get().get<HybridNode>(source);
        node.execute(pipe, program, i);
      }
      else
      {
        pushScalar(p, def.format.scalarSubType);
      }
      optionIdx++;
      break;
    case DataType::eFloat:
    case DataType::eFloat2:
    case DataType::eInt:
    case DataType::eInt2:
      pushScalar(p, def.format.type);
      break;
    case DataType::eBool:
      optionIdx++;
      break;
    case DataType::eEnum:
      optionIdx += def.maxEnum;
      break;
    }
  }
  pushOutputs(pipe, program);
  program.run();
}

} // namespace terra