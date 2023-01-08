
#include "hyb/HybridNode.h"
#include "GpuMinMax.h"
#include "Logger.h"
#include "Terra.h"
#include "hyb/GpuDataSource.h"
#include "hyb/HybridNodeMeta.h"
#include "hyb/HybridPipeline.h"

namespace terra
{
//============== ClassicHybridNode ==================

bool ClassicHybridNode::preExecute(HybridPipeline& pipe, std::vector<Parameter>& parameters)
{
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  parameters.clear();
  parameters.reserve((size_t)std::popcount(gpuMeta.preParams));
  return forEachBit(
    [&](auto i)
    {
      parameters.emplace_back(gpuMeta.parameterDef[i].getDefault());
      auto idx = (uint32_t)gpuMeta.parameterDef[i].format.semantic;
      if (!gpuMeta.autoRegistry[idx].pre(pipe, *this, i, parameters.back()))
        return false;

      return true;
    },
    gpuMeta.preParams);
}

HybridNode::Result ClassicHybridNode::postExecute(HybridPipeline& pipe)
{
  auto const&        gpuMeta = static_cast<GpuNodeMeta const&>(meta);
  HybridNode::Result result  = HybridNode::Result::eDone;
  if (!forEachBit(
        [&](auto i)
        {
          auto idx = gpuMeta.parameterDef[i].format.semantic;
          switch (gpuMeta.autoRegistry[idx.id].post(pipe, *this, i))
          {
          case AutoParam::eReportFailure:
            result = HybridNode::Result::eFailed;
            return false;
          case AutoParam::eContinueIteration:
            result = HybridNode::Result::eContinue;
            break;
          }
          return true;
        },
        gpuMeta.postParams))
    return result;

  forEachBit(
    [&](auto i)
    {
      uint32_t idx = (uint32_t)gpuMeta.outputs[i].format.semantic;
      switch (gpuMeta.autoRegistry[idx].post(pipe, *this, i))
      {
      case AutoParam::eReportFailure:

        result = HybridNode::Result::eFailed;
        return false;
      case AutoParam::eContinueIteration:
        result = HybridNode::Result::eContinue;
        break;
      }
      return true;
    },
    gpuMeta.autoOutputs);
  return result;
}

//============== GpuNode ==================
bool GpuNode::prepare(HybridPipeline& pipe)
{
  ProgramKey  option;
  HashMachine machine{0};
  pipe.push(self);
  probe(pipe, option, machine);

  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    passCount = gpuMeta.getNumPasses();
  option.hash           = machine.value;
  option.meta           = meta.id;
  auto program          = ndat.gpuPasses;
  if (!program || ndat.key != option)
    program = gpuMeta.findProgram(option);
  if (program)
  {
    ndat.key       = option;
    ndat.gpuPasses = program;
    createResources(ndat, pipe, gpuMeta);
    return true;
  }

  // Rebuild
  GpuPipelinePtr newProgram = std::make_shared<GpuPipeline>();
  newProgram->passes.reserve(passCount);
  for (uint32_t pass = 0; pass < passCount; ++pass)
  {
    {
      auto builder = get().getDevice().createSourceBuilder(ShaderLang::eGLSL, gpuMeta.passes[pass].type ==
                                                                                  GpuNodeMeta::PassType::eFullscreen
                                                                                ? SourceType::eFullscreenGraphNode
                                                                                : SourceType::eComputeProgram);
      build(pipe, pass, *builder);
      // use GpuProgramBuilder to build a program
      newProgram->passes.emplace_back(std::move(builder->finalize()));
      if (!newProgram->passes.back()->program)
      {
        logError("Failed to compile program: {}", gpuMeta.getCode(pass).function);
        return false;
      }
    }
  }
  gpuMeta.addProgram(option, newProgram);
  ndat.key       = option;
  ndat.gpuPasses = std::move(newProgram);
  createResources(ndat, pipe, gpuMeta);
  return true;
}

void GpuNode::createResources(GpuNode::Data& ndat, HybridPipeline& pipe, GpuNodeMeta const& gpuMeta)
{
  ndat.outputs.resize(gpuMeta.outputs.size());
  for (uint32_t i = 0, e = (uint32_t)gpuMeta.outputs.size(); i < e; ++i)
  {
    ndat.outputs[i] = pipe.declareBuffer();
    pipe.describeImage(ndat.outputs[i], getSelf(), pipe.size().x, pipe.size().y, meta.outputs[i].format.imageFormat);
    // declare output buffers upfront
    if (gpuMeta.outputs[i].format.semantic)
      pipe.setBuffer(gpuMeta.outputs[i].format.semantic, ndat.outputs[i], getSelf());
  }
}

void GpuNode::build(HybridPipeline& pipe, uint32_t pass, SourceBuilder& builder)
{
  auto const&   gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t      optionIdx = 0;
  ShaderOptions shoption{gpuMeta.getDictionaryIdx()};

  enum class Action : uint8_t
  {
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
      auto outid  = std::get<Source>(p).secondary;
      if (DataSource::isPushable(source, pipe.tile(), constraintTileStart, constraintTileCount))
      {
        //
        shoption.setOption(optionIdx);
      }
      else
        paramAct = Action::ePushConstant;
    }
    else if (std::holds_alternative<BufferRef>(p))
    {
      auto r = std::get<BufferRef>(p);
      if (r && r->getSize() > 0)
        shoption.setOption(optionIdx);
      else
        paramAct = Action::ePushConstant;
    }
    else
      paramAct = Action::ePushConstant;

    switch (def.format.type)
    {
    case DataTypeEnum::eBuffer:
    {
      auto r = std::get<BufferRef>(p);
      if (paramAct == Action::ePushConstant)
        builder.scalar(def.name(), def.format);
      else
        builder.param(def.name(), def.format);
      optionIdx++;
      break;
    }
    case DataTypeEnum::eCurveData:
    case DataTypeEnum::eImage:
    case DataTypeEnum::eInput:
    case DataTypeEnum::ePostProcess:
    case DataTypeEnum::eSource:
      switch (paramAct)
      {
      case Action::eSampleNode:
        builder.param(def.name(), def.format);
        break;
      case Action::ePushConstant:
        builder.scalar(def.name(), def.format);
        break;
      }
      optionIdx++;
      break;
    case DataTypeEnum::eInt:
    case DataTypeEnum::eUint:
    case DataTypeEnum::eFloat:
    case DataTypeEnum::eFloat2:
    case DataTypeEnum::eInt2:
    case DataTypeEnum::eUint2:
    case DataTypeEnum::eFloat3:
    case DataTypeEnum::eFloat4:
    case DataTypeEnum::eMat4:
    case DataTypeEnum::eArray:
      builder.param(def.name(), def.format);
      break;
    case DataTypeEnum::eBool:
      if (std::get<bool>(p))
        shoption.setOption(optionIdx);
      optionIdx++;
      break;
    case DataTypeEnum::eEnum:
      shoption.setOption(optionIdx + (uint32_t)std::get<int>(p));
      optionIdx += def.maxEnum;
      break;
    }
  }
  for (auto o : code.outputs)
    builder.output(meta.outputs[o].name(), meta.outputs[o].format);

  builder.options(shoption);
  builder.pushExtension(code.extensions);
  builder.append(code.shaderContent);
  if (!code.function.empty())
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
      if (DataSource::isPushable(source, pipe.tile(), constraintTileStart, constraintTileCount))
      {
        //
        if (DataSource::isNode(source))
        {
          auto& node = get().get<GpuNode>(source);
          node.prepare(pipe);
        }
        option.active++;
        shoption.setOption(optionIdx);
        optionIdx++;
      }
    }
    else if (std::holds_alternative<BufferRef>(p))
    {
      auto r = std::get<BufferRef>(p);
      if (r && r->getSize() > 0)
      {
        option.active++;
        shoption.setOption(optionIdx);
      }
      optionIdx++;
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
      case DataTypeEnum::eSource:
        optionIdx++;
        break;
      case DataTypeEnum::eBool:
        if (std::get<bool>(p))
          shoption.setOption(optionIdx);
        optionIdx++;
        break;
      case DataTypeEnum::eEnum:
        shoption.setOption(optionIdx + (uint32_t)std::get<int>(p));
        optionIdx += def.maxEnum;
        break;
      }
    }
  }

  machine(shoption.options.mask);
  option.options     = shoption;
  ndat.activeOptions = shoption;
}

void GpuNode::executeImpl(HybridPipeline& pipe, std::vector<Parameter>& autoParam)
{
  auto&       ndat     = nodeData[pipe.id()];
  auto const& gpuMeta  = static_cast<GpuNodeMeta const&>(meta);
  auto        autopVal = autoParam.begin();

  auto paramAccess = [&](uint32_t i) -> Parameter
  {
    if ((1ull << i) & gpuMeta.preParams)
    {
      Parameter p = (*autopVal++);
      return p;
    }
    return param(i);
  };

  uint32_t passCount = gpuMeta.getNumPasses();
  auto&    gpuPipe   = ndat.gpuPasses;
  for (uint32_t pass = 0; pass < passCount; ++pass)
  {
    auto const& code = gpuMeta.getCode(pass);

    gpuPipe->passes[pass]->touch();
    auto state          = code.state;
    state.viewport.size = pipe.size();
    auto     program    = ShaderProgramInstance(state, *gpuPipe->passes[pass], pipe,
                                                gpuMeta.passes[pass].type == GpuNodeMeta::PassType::eCompute);
    uint32_t optionIdx  = 0;
    // check if any conditions have changed
    for (auto i : code.parameters)
    {
      auto const& def = meta.parameterDef[i];
      auto        p   = paramAccess(i);

      switch (def.format.type)
      {
      case DataTypeEnum::eBuffer:
        if (ndat.activeOptions.isSet(optionIdx))
        {
          auto source = std::get<BufferRef>(p);
          program.pushBuffer(source->buffer(), source->getSize(), def.format);
        }
        else
        {
          program.pushValue(p, def.format.scalarSubType, def.format.scalarSubType);
        }
        optionIdx++;
        break;
      case DataTypeEnum::eCurveData:
      case DataTypeEnum::eImage:
      case DataTypeEnum::eInput:
      case DataTypeEnum::ePostProcess:
      case DataTypeEnum::eSource:
        if (ndat.activeOptions.isSet(optionIdx))
        {
          auto  source = std::get<Source>(p);
          auto& node   = get().get<HybridNode>(source.source);
          node.push(pipe, program, source.secondary, def.format);
        }
        else
        {
          program.pushValue(p, def.format.scalarSubType, def.format.scalarSubType);
        }
        optionIdx++;
        break;
      case DataTypeEnum::eFloat:
      case DataTypeEnum::eFloat2:
      case DataTypeEnum::eInt:
      case DataTypeEnum::eInt2:
      case DataTypeEnum::eUint:
      case DataTypeEnum::eUint2:
      case DataTypeEnum::eFloat3:
      case DataTypeEnum::eFloat4:
      case DataTypeEnum::eMat4:
      case DataTypeEnum::eArray:
        program.pushValue(p, def.format.type, def.format.scalarSubType);
        break;
      case DataTypeEnum::eBool:
        optionIdx++;
        break;
      case DataTypeEnum::eEnum:
        optionIdx += def.maxEnum;
        break;
      }
    }
    pushOutputs(pipe, pass, program);
    program.run();
  }
}

void GpuNode::push(HybridPipeline& pipe, ShaderProgramInstance& program, uint32_t outIdx, DataFormat inFmt)
{
  auto& ndat = nodeData[pipe.id()];
  program.pushValue(ndat.outputs[outIdx], inFmt);
}

void GpuNode::pushOutputs(HybridPipeline& pipe, uint32_t pass, ShaderProgramInstance& program)
{
  auto&       ndat    = nodeData[pipe.id()];
  auto const& gpuMeta = static_cast<GpuNodeMeta const&>(meta);

  auto const& code = gpuMeta.getCode(pass);
  for (uint32_t i : code.outputs)
  {
    program.pushOutput(ndat.outputs[i], meta.outputs[i].format, meta.outputs[i].clear, meta.outputs[i].clearValue);
  }
}

HybridNode::Result GpuNode::postExecute(HybridPipeline& pipe)
{
  return ClassicHybridNode::postExecute(pipe);
}

//============== GpuScriptNode ==================
GpuScriptNode::GpuScriptNode(NodeMeta const& m) : GpuNode(m)
{
  parameters.reserve(m.parameterDef.size());
  for (size_t i = 0; i < m.parameterDef.size(); ++i)
  {
    auto format = m.parameterDef[i].format;
    parameters.emplace_back(m.parameterDef[i].getDefault());
  }
  forEachBit(
    [&](auto i)
    {
      auto idx = (uint32_t)meta.parameterDef[i].format.semantic;
      meta.autoRegistry[idx].change(*this, i);
      return true;
    },
    meta.depParams);
}

void GpuScriptNode::set(uint32_t i, Parameter const& param)
{
  parameters[i] = param;
}

Parameter GpuScriptNode::get(uint32_t i) const
{
  return parameters[i];
}
//============== GpuImageNode ==================
GpuImageNode::GpuImageNode(NodeMeta const& m) : GpuNode(m)
{
  image = get().add(std::make_shared<GpuImage>());
}

GpuImageNode::~GpuImageNode()
{
  get().destroy(image);
}

void GpuImageNode::executeImpl(HybridPipeline& pipe, std::vector<Parameter>&)
{
  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    passCount = gpuMeta.getNumPasses();
  auto&       gpuPipe   = ndat.gpuPasses;

  auto const& code = gpuMeta.getCode(0);

  gpuPipe->passes[0]->touch();
  auto state          = code.state;
  state.viewport.size = pipe.size();
  auto     program    = ShaderProgramInstance(state, *gpuPipe->passes[0], pipe, false);
  uint32_t optionIdx  = 0;
  auto&    imageObj   = get().get<GpuImage>(image);
  if (imageObj.isPushable())
  {
    imageObj.upload();
    auto fmt        = meta.parameterDef[0].format;
    fmt.imageFormat = imageObj.format;
    program.pushImage(imageObj.getHandle(version), fmt);
  }
  else
  {
    program.pushValue(float(1.0f));
  }
  program.pushValue(sampleScale);
  program.pushValue(sampleOffset);
  program.pushValue(scale);
  pushOutputs(pipe, 0, program);
  program.run();
  // if (imageObj.isPushable())
  //   imageObj.destroyHandle();
}

void GpuImageNode::selfUpdated()
{
  auto& node = get().get<GpuImage>(image);
  node.add(getSelf());
}

//============== GpuCurveNode ==================
GpuCurveNode::GpuCurveNode(NodeMeta const& m) : GpuNode(m)
{
  curve = get().add(std::make_shared<GpuCurveData>());
  scale = vec2(1.f, 1.f);
}

GpuCurveNode::~GpuCurveNode()
{
  get().destroy(curve);
}

void GpuCurveNode::executeImpl(HybridPipeline& pipe, std::vector<Parameter>&)
{
  auto&       ndat      = nodeData[pipe.id()];
  auto const& gpuMeta   = static_cast<GpuNodeMeta const&>(meta);
  uint32_t    passCount = gpuMeta.getNumPasses();
  auto&       gpuPipe   = ndat.gpuPasses;

  auto const& code = gpuMeta.getCode(0);

  gpuPipe->passes[0]->touch();
  auto state          = code.state;
  state.viewport.size = pipe.size();
  auto     program    = ShaderProgramInstance(state, *gpuPipe->passes[0], pipe, false);
  uint32_t optionIdx  = 0;
  auto&    curveObj   = get().get<GpuCurveData>(curve);
  program.pushBuffer(curveObj.getHandle(version), curveObj.size(), meta.parameterDef[0].format);
  program.pushValue(scale);
  pushOutputs(pipe, 0, program);
  program.run();
}

void GpuCurveNode::selfUpdated()
{
  auto& node = get().get<DataSource>(curve);
  node.add(getSelf());
}

} // namespace terra