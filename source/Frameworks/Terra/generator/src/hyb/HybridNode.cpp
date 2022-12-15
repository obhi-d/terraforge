
#include "hyb/HybridNode.h"
#include "Logger.h"
#include "Terra.h"
#include "hyb/HybridNodeMeta.h"
#include "hyb/HybridPipeline.h"

namespace terra
{
//============== ClassicHybridNode ==================

bool ClassicHybridNode::preExecute(HybridPipeline& pipe)
{
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  for (auto i : gpuMeta.autoParams)
  {
    uint32_t idx = (uint32_t)gpuMeta.parameterDef[i].format.semantic;
    if (gpuMeta.autoRegistry[idx].pre)
    {
      if (gpuMeta.autoRegistry[idx].pre(pipe, *this, i) == AutoParam::eReportFailure)
        return false;
    }
  }
  for (auto i : gpuMeta.autoOutputs)
  {
    uint32_t idx = (uint32_t)gpuMeta.outputs[i].format.semantic;
    if (gpuMeta.autoRegistry[idx].pre)
    {
      if (gpuMeta.autoRegistry[idx].pre(pipe, *this, i) == AutoParam::eReportFailure)
        return false;
    }
  }
  return true;
}

HybridNode::Result ClassicHybridNode::postExecute(HybridPipeline& pipe)
{
  auto const&        gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  HybridNode::Result result  = HybridNode::Result::eDone;
  for (auto i : gpuMeta.autoParams)
  {
    uint32_t idx = (uint32_t)gpuMeta.parameterDef[i].format.semantic;
    if (gpuMeta.autoRegistry[idx].post)
    {
      switch (gpuMeta.autoRegistry[idx].post(pipe, *this, i))
      {
      case AutoParam::eReportFailure:
        return HybridNode::Result::eFailed;
      case AutoParam::eContinueIteration:
        result = HybridNode::Result::eContinue;
        break;
      }
    }
  }
  for (auto i : gpuMeta.autoOutputs)
  {
    uint32_t idx = (uint32_t)gpuMeta.outputs[i].format.semantic;
    if (gpuMeta.autoRegistry[idx].post)
    {
      switch (gpuMeta.autoRegistry[idx].post(pipe, *this, i))
      {
      case AutoParam::eReportFailure:
        return HybridNode::Result::eFailed;
      case AutoParam::eContinueIteration:
        result = HybridNode::Result::eContinue;
        break;
      }
    }
  }
  return result;
}

//============== GpuNode ==================
bool GpuNode::prepare(HybridPipeline& pipe)
{
  ProgramKey  option;
  HashMachine machine{0};
  probe(pipe, option, machine);
  pipe.push(self);

  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    passCount = gpuMeta.getNumPasses();
  option.hash           = machine.value;
  auto program          = ndat.gpuPasses.lock();
  if (!program || ndat.key != option)
    program = gpuMeta.findProgram(option);
  if (program)
  {
    ndat.key       = option;
    ndat.gpuPasses = program;
    return true;
  }

  // Rebuild
  GpuPipelinePtr newProgram = std::make_shared<GpuPipeline>();
  newProgram->passes.reserve(passCount);
  for (uint32_t pass = 0; pass < passCount; ++pass)
  {
    {
      auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, SourceType::eFullscreenGraphNode);
      build(pipe, pass, *builder);
      // use GpuProgramBuilder to build a program
      newProgram->passes[pass] = builder->finalize();
      if (!newProgram->passes[pass].material.program)
      {
        logError("Failed to compile program: {}", gpuMeta.getCode(pass).function);
        return false;
      }
    }
  }
  gpuMeta.addProgram(option, newProgram);
  ndat.key       = option;
  ndat.gpuPasses = std::move(newProgram);
  ndat.outputs.resize(gpuMeta.outputs.size());
  return true;
}

bool GpuNode::isSourceModifier() const
{
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  return gpuMeta.isSourceModifier;
}

void GpuNode::build(HybridPipeline& pipe, uint32_t pass, SourceBuilder& builder)
{
  auto const&   gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t      optionIdx = 0;
  ShaderOptions shoption{gpuMeta.getDictionaryIdx()};

  enum class Action : uint8_t
  {
    eComputeNode,
    ePushConstant,
    eSampleNode
  };

  auto& code = gpuMeta.getCode(pass);
  for (auto i : code.parameters)
  {
    auto const& def      = meta.parameterDef[i];
    auto        p        = param(i);
    Action      paramAct = Action::eSampleNode;
    if (std::holds_alternative<Source>(p))
    {
      auto source = std::get<Source>(p).source;
      if (DataSource::isValid(source) &&
          DataSource::isWithinTile(pipe.tile(), constraintTileStart, constraintTileCount))
      {
        //
        auto& node = get().get<GpuNode>(source);
        if (node.isSourceModifier())
        {
          auto oldid = builder.swapId(optionIdx + 1);
          node.build(pipe, 0, builder);
          builder.swapId(oldid);
          paramAct = Action::eComputeNode;
        }
        shoption.setOption(optionIdx);
      }
      else
        paramAct = Action::ePushConstant;
    }

    switch (def.format.type)
    {
    case DataTypeEnum::eCurveData:
    case DataTypeEnum::eImage:
    case DataTypeEnum::eInput:
    case DataTypeEnum::ePostProcess:
    case DataTypeEnum::eBuffer:
      switch (paramAct)
      {
      case Action::eSampleNode:
        builder.sampleParam(def.name(), def.format);
        break;
      case Action::eComputeNode:
        builder.computeParam(def.name(), def.format);
        break;
      case Action::ePushConstant:
        builder.sampleScalar(def.name(), def.format);
        break;
      }
      optionIdx++;
      break;
    case DataTypeEnum::eFloat:
    case DataTypeEnum::eFloat2:
    case DataTypeEnum::eInt:
    case DataTypeEnum::eInt2:
      builder.sampleScalar(def.name(), def.format);
      break;
    case DataTypeEnum::eBool:
      if (std::get<ScalarValue>(p).bvalue)
        shoption.setOption(optionIdx);
      optionIdx++;
      break;
    case DataTypeEnum::eEnum:
      shoption.setOption(optionIdx + (uint32_t)std::get<ScalarValue>(p).ivalue);
      optionIdx += def.maxEnum;
      break;
    }
  }
  if (!isSourceModifier())
  {
    for (auto o : code.outputs)
      builder.writeOutput(meta.outputs[o].name(), meta.outputs[o].format);
  }

  builder.pushExtension(code.extensions);
  builder.append(code.shaderContent);
  builder.call(code.function);
}

void GpuNode::probe(HybridPipeline& pipe, ProgramKey& option, HashMachine& machine)
{
  if (pipe.id() >= nodeData.size())
    nodeData.resize(pipe.id() + 1);

  auto& ndat = nodeData[pipe.id()];

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
          DataSource::isWithinTile(pipe.tile(), constraintTileStart, constraintTileCount))
      {
        //

        auto& node = get().get<GpuNode>(source);
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
      case DataTypeEnum::eCurveData:
      case DataTypeEnum::eImage:
      case DataTypeEnum::eInput:
      case DataTypeEnum::ePostProcess:
      case DataTypeEnum::eBuffer:
        optionIdx++;
        break;
      case DataTypeEnum::eBool:
        if (std::get<ScalarValue>(p).bvalue)
          shoption.setOption(optionIdx);
        optionIdx++;
        break;
      case DataTypeEnum::eEnum:
        shoption.setOption(optionIdx + (uint32_t)std::get<ScalarValue>(p).ivalue);
        optionIdx += def.maxEnum;
        break;
      }
    }
  }

  machine(shoption.options.mask);
  option.options     = shoption;
  ndat.activeOptions = shoption;
}

void GpuNode::executeImpl(HybridPipeline& pipe)
{
  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    passCount = gpuMeta.getNumPasses();
  auto        gpuPipe   = ndat.gpuPasses.lock();
  for (uint32_t pass = 0; pass < passCount; ++pass)
  {
    auto const& code = gpuMeta.getCode(pass);

    auto     program   = ShaderProgramInstance(gpuPipe->passes[pass], pipe);
    uint32_t optionIdx = 0;
    // check if any conditions have changed
    for (auto i : code.parameters)
    {
      if (GpuNodeMeta::kOutputMask & i)
      {
        i               = i & ~GpuNodeMeta::kOutputMask;
        auto const& def = meta.outputs[i];
        switch (def.format.type)
        {
        case DataTypeEnum::eCurveData:
        case DataTypeEnum::eImage:
        case DataTypeEnum::eInput:
        case DataTypeEnum::ePostProcess:
        case DataTypeEnum::eBuffer:
          program.pushValue(ndat.outputs[i], def.format);
          optionIdx++;
          break;
        }
      }
      else
      {
        auto const& def = meta.parameterDef[i];
        auto        p   = param(i);

        switch (def.format.type)
        {
        case DataTypeEnum::eCurveData:
        case DataTypeEnum::eImage:
        case DataTypeEnum::eInput:
        case DataTypeEnum::ePostProcess:
        case DataTypeEnum::eBuffer:
          if (ndat.activeOptions.isSet(optionIdx))
          {
            auto  source = std::get<Source>(p);
            auto& node   = get().get<HybridNode>(source.source);
            node.push(pipe, program, i, source.secondary);
          }
          else
          {
            program.pushValue(std::get<ScalarValue>(p), def.format.scalarSubType);
          }
          optionIdx++;
          break;
        case DataTypeEnum::eFloat:
        case DataTypeEnum::eFloat2:
        case DataTypeEnum::eInt:
        case DataTypeEnum::eInt2:
          program.pushValue(std::get<ScalarValue>(p), def.format.scalarSubType);
          break;
        case DataTypeEnum::eBool:
          optionIdx++;
          break;
        case DataTypeEnum::eEnum:
          optionIdx += def.maxEnum;
          break;
        }
      }
    }
    if (!isSourceModifier())
      pushOutputs(pipe, pass, program);
    program.run();
  }
}

void GpuNode::push(HybridPipeline& pipe, ShaderProgramInstance& program, uint32_t paramIdx, uint32_t outIdx)
{
  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    optionIdx = 0;

  if (isSourceModifier())
  {
    for (uint32_t i = 0; i < meta.parameterDef.size(); ++i)
    {
      auto const& def = meta.parameterDef[i];
      auto        p   = param(i);

      switch (def.format.type)
      {
      case DataTypeEnum::eCurveData:
      case DataTypeEnum::eImage:
      case DataTypeEnum::eInput:
      case DataTypeEnum::ePostProcess:
      case DataTypeEnum::eBuffer:
        if (ndat.activeOptions.isSet(optionIdx))
        {
          auto  source = std::get<Source>(p);
          auto& node   = get().get<HybridNode>(source.source);
          node.push(pipe, program, i, source.secondary);
        }
        else
        {
          program.pushValue(std::get<ScalarValue>(p), def.format.scalarSubType);
        }
        optionIdx++;
        break;
      case DataTypeEnum::eFloat:
      case DataTypeEnum::eFloat2:
      case DataTypeEnum::eInt:
      case DataTypeEnum::eInt2:
        program.pushValue(std::get<ScalarValue>(p), def.format.scalarSubType);
        break;
      case DataTypeEnum::eBool:
        optionIdx++;
        break;
      case DataTypeEnum::eEnum:
        optionIdx += def.maxEnum;
        break;
      }
    }
  }
  else
  {
    program.pushValue(ndat.outputs[outIdx], meta.parameterDef[outIdx].format);
  }
}

void GpuNode::pushOutputs(HybridPipeline& pipe, uint32_t pass, ShaderProgramInstance& program)
{
  auto&       ndat    = nodeData[pipe.id()];
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);

  auto const& code = gpuMeta.getCode(pass);
  for (uint32_t i : code.outputs)
  {
    if (!ndat.outputs[i])
    {
      switch (gpuMeta.outputs[i].format.semantic)
      {
      case SemanticEnum::eHeights:
        ndat.outputs[i] = pipe.heights();
        break;
      case SemanticEnum::eLayerContrib:
        ndat.outputs[i] = pipe.layerContrib();
        break;
      default:
        if (!ndat.outputs[i])
        {
          ndat.outputs[i] = pipe.declareBuffer();
          pipe.describeImage(ndat.outputs[i], getSelf(), pipe.size().x, pipe.size().y,
                             meta.outputs[i].format.imageFormat);
        }
      }
    }
    program.pushOutput(ndat.outputs[i], meta.outputs[i].format);
  }
}

HybridNode::Result GpuNode::postExecute(HybridPipeline& pipe)
{
  return ClassicHybridNode::postExecute(pipe);
}

} // namespace terra