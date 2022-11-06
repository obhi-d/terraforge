
#include "Node.h"
#include "Logger.h"
#include "Pipeline.h"
#include "ShaderBuilder.h"
#include "Terra.h"
#include "Updater.h"
#include <charconv>
#include <numeric>

namespace terra
{

bool DataFormat::isCompatible(DataFormat const& from, DataFormat const& to)
{
  switch (from.type)
  {
  case DataType::eInt2:
  case DataType::eFloat2:
  case DataType::eInt:
  case DataType::eFloat:
  case DataType::eCurveData:
  case DataType::eBool:
  case DataType::eEnum:
    return from.type == to.type;
  //case DataType::eImageSource:
  case DataType::eImage:
    return (to.type == DataType::eImage) && from.scalarSubType == to.scalarSubType;
  case DataType::eBuffer:
    return from.type == to.type && from.scalarSubType == to.scalarSubType;
  }
  return false;
}

std::string_view typeToString(DataType type)
{
  switch (type)
  {
  case DataType::eEnum:
    return "enum";
  case DataType::eInt:
    return "int";
  case DataType::eInt2:
    return "ivec2";
  case DataType::eFloat:
    return "float";
  case DataType::eFloat2:
    return "vec2";
  case DataType::eBool:
    return "bool";
  case DataType::eImage:
    return "image";
  case DataType::eBuffer:
    return "source";
  case DataType::eCurveData:
    return "curve";
  }
  return "invalid";
}

DataType stringToType(std::string_view stype)
{
  DataType type = DataType::eInvalid;
  if (stype == "enum")
    type = DataType::eEnum;
  else if (stype == "int")
    type = DataType::eInt;
  else if (stype == "float")
    type = DataType::eFloat;
  else if (stype == "ivec2")
    type = DataType::eInt2;
  else if (stype == "vec2")
    type = DataType::eFloat2;
  else if (stype == "bool")
    type = DataType::eBool;
  else if (stype == "image")
    type = DataType::eImage;
  else if (stype == "buffer" || stype == "source")
    type = DataType::eBuffer;
  else if (stype == "curve")
    type = DataType::eCurveData;
  return type;
}

bool ParameterMeta::canBeSource() const
{
  switch (format.type)
  {
  case DataType::eCurveData:
  case DataType::eBuffer:
  case DataType::eImage:
    return true;
  }
  return false;
}

bool ParameterMeta::canBeScalar() const
{
  return true;
}

ScalarValue ParameterMeta::getDefault() const
{
  if (DataType::eBool == format.type)
    return ScalarValue((bool)(values[ValueType::eDefault].ival != 0));
  else if(DataType::eEnum == format.type) 
    return ScalarValue(values[ValueType::eDefault].ival, 0);
  switch (format.scalarSubType)
  {
  case DataType::eFloat:
    return ScalarValue(values[ValueType::eDefault].fval);
  case DataType::eFloat2:
    return vec2{values[ValueType::eDefault].fval, values[ValueType::eDefault].fval};
  case DataType::eBool:
  case DataType::eInt:
    return ScalarValue(values[ValueType::eDefault].ival);
  case DataType::eInt2:
    return ivec2{values[ValueType::eDefault].ival, values[ValueType::eDefault].ival};
  default:
    return ScalarValue();
  }
}

void ParameterMeta::setTypeFromString(std::string_view stype)
{
  format.type = stringToType(stype);
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto setter = [this, valType](auto value)
  {
    values[valType] = DataValue(value);
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (format.type)
  {
  case DataType::eEnum:
  case DataType::eBool:
  case DataType::eInt2:
  case DataType::eInt:
    if (value == "inf")
      ivalue = std::numeric_limits<int>::max();
    else if (value == "-inf")
      ivalue = std::numeric_limits<int>::min();
    else if (value == "true")
      ivalue = 1;
    else if (value == "false")
      ivalue = 0;
    else
      std::from_chars(value.data(), value.data() + value.size(), ivalue);
    setter(ivalue);
    break;
  case DataType::eCurveData:
  case DataType::eImage:
  case DataType::eBuffer:
  case DataType::eFloat:
  case DataType::eFloat2:
    if (value == "inf")
      fvalue = std::numeric_limits<float>::max();
    else if (value == "-inf")
      fvalue = std::numeric_limits<float>::min();
    else
      std::from_chars(value.data(), value.data() + value.size(), fvalue);
    setter(fvalue);
    break;
  }
}

/*
bool ParameterMeta::affectsOptions() const
{
  switch (format.type)
  {
  case DataType::eCurveData:
  case DataType::eImage:
  case DataType::eBuffer:
  case DataType::eBool:
  case DataType::eEnum:
    return true;
  }
  return false;
}

bool ParameterMeta::modifyOptions(Pipeline& pipe, Parameter const& p, Options& opt) const
{
  if (std::holds_alternative<Source>(p))
  {
    auto node = std::get<Source>(p);
    if (node.source && DataSource::isValid(node.source))
    {
      DataSource& ds = get().get<DataSource>(node.source);
      if (!ds.ensure(pipe))
        return false;
      if (ds.isEnabled(pipe))
        opt |= 1ull << (Options)optionIndex;
    }
  }
  else
  {
    switch (format.type)
    {
    case DataType::eBool:
      if (std::get<ScalarValue>(p).bvalue)
        opt |= 1ull << (Options)optionIndex;
      break;
    case DataType::eEnum:
    {
      auto opt = std::get<ScalarValue>(p).ivalue;
      assert(opt >= 0 && opt < optionCount);
      opt |= 1ull << ((Options)optionIndex + (Options)opt);
    }
    break;
    }
  }
  return true;
}

void NodeMeta::buildShaderGLSL(ShaderContent const& nodeContent)
{
  auto&       main          = Terra::get();
  auto&       rd            = main.getDevice();
  auto        caps          = rd.getCaps();
  auto const& commonContent = main.getShaderContent(ShaderLang::eGLSL);
  shaderBuilder             = rd.createShaderBuilder(ShaderLang::eGLSL);

  shaderBuilder->begin(ShaderType::eCompute);
  shaderBuilder->beginSection(ShaderBuilder::eDecl);
  shaderBuilder->append(nodeContent.extensions);
  shaderBuilder->append(std::format("#define Binding_Node {}\n"
                                    "#define WorkGroupSize {}\n"
                                    "#define output_t {}\n",
                                    nodeContent.function, get().getWorkGroupSize(),
                                    glsl::bufferWriteType(format.scalarSubType)));
  shaderBuilder->append(commonContent.typesAndConstants);

  std::string generated;
  std::string nodeParams;
  // generated content
  int32_t paramOffsets = sizeof(EnvParams);
  auto    declOrder    = std::vector<uint32_t>(parameterDef.size());

  std::iota(declOrder.begin(), declOrder.end(), 0);
  std::sort(declOrder.begin(), declOrder.end(),
            [this](uint32_t first, uint32_t second)
            {
              return parameterDef[first].format.type < parameterDef[second].format.type;
            });

  std::vector<GfxDescriptorSetLayout::Descriptor> descriptorSetBindings;
  using DT = GfxDescriptorSetLayout::DescriptorType;

  // print only constants
  for (auto o : declOrder)
  {
    auto& t = parameterDef[o];
    switch (t.format.type)
    {
    case DataType::eInt:
      nodeParams += "  int ";
      nodeParams += t.id;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case DataType::eFloat:
      nodeParams += "  float ";
      nodeParams += t.id;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case DataType::eInt2:
      nodeParams += "  ivec2 ";
      nodeParams += t.id;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case DataType::eFloat2:
      nodeParams += "  vec2 ";
      nodeParams += t.id;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case DataType::eEnum:
      t.optionIndex = (int16_t)options.size();
      for(int i = 0; i < t.optionCount; ++i)
        options.push_back(std::format("{}_OptionEnabled_{}", t.id, i));      
      break;
    case DataType::eBool:
      t.optionIndex = (int16_t)options.size();
      options.push_back(t.id + "_Enabled");
      generated += std::format("const bool {0} = bool({0}_Enabled);\n", t.id);
      break;
    case DataType::eBuffer:
      glsl::declBufferSource(nodeParams, generated, descriptorSetBindings, options, t, paramOffsets, *shaderBuilder);
      break;
    case DataType::eImage:
      glsl::declImageSource(nodeParams, generated, descriptorSetBindings, options, t, paramOffsets, *shaderBuilder);
      break;
    case DataType::eCurveData:
      glsl::declCurveData(nodeParams, generated, descriptorSetBindings, options, t, paramOffsets, *shaderBuilder);
      break;
    }
  }
  // deal with buffers and textures
  if (hasTextureOutput)
  {
    glsl::declTextureOutput(generated, descriptorSetBindings, options, outputDescriptorIdx, *shaderBuilder);
  }
  else
  {
    glsl::declBufferOutput(generated, descriptorSetBindings, options, outputDescriptorIdx, *shaderBuilder);
  }

  if (outputDownscale > 1)
    shaderBuilder->append(std::format("#define OutputDownscale {}\n", outputDownscale));
  else if (outputUpscale > 1)
    shaderBuilder->append(std::format("#define OutputUpscale {}\n", outputUpscale));

  uboSize     = paramOffsets;
  hasUniforms = (!nodeParams.empty() && paramOffsets != sizeof(EnvParams));

  if (hasUniforms)
  {
    shaderBuilder->append("#define NodeUniforms_Enabled 1\n"
                          "struct NodeUniforms\n{");
    shaderBuilder->append(nodeParams);
    shaderBuilder->append("};\n");
  }
  else
    shaderBuilder->append("#define NodeUniforms_Enabled 0\n");

  shaderBuilder->append(commonContent.fixedResources);
  shaderBuilder->append(commonContent.utilityFunctions);
  shaderBuilder->append(generated);
  shaderBuilder->append("\n");
  {
    auto bi = shaderBuilder->declConstants("U", "Constants");
    shaderBuilder->append(bi.content);
    shaderBuilder->append("{ NodeParams params; }constants;\n");
    constantsDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(bi.descriptor);
  }
  if (options.size() >= 64)
  {
    logError("Too many options for : {}", id);
  }
  shaderBuilder->endSection();
  shaderBuilder->beginSection(ShaderBuilder::eMain);
  shaderBuilder->append(nodeContent.shaderContent);
  shaderBuilder->append(commonContent.main);
  shaderBuilder->endSection();
  shaderBuilder->end();
  nbDescriptors       = (int)descriptorSetBindings.size();
  descriptorSetLayout = main.getDevice().createDescriptorSetLayout(descriptorSetBindings);
}

GfxProgram::handle NodeMeta::getShaderGLSL(Options optionBitSet) const
{
  auto& main = Terra::get();
  auto& rd   = main.getDevice();
  auto  it   = shaders.find(optionBitSet);
  if (it == shaders.end())
  {
    auto shader = rd.createProgram(ShaderOptions{.names = options, .bitMask = optionBitSet}, *shaderBuilder);
    if (!shader)
    {
      logError("Failed to compile shader.");
      return {};
    }
    rd.applyLayoutToProgram(shader, this->descriptorSetLayout);
    shaders.emplace(optionBitSet, shader);
    return shader;
  }
  else
    return it->second;
}

NodeMeta::~NodeMeta()
{
  destroy();
}

void NodeMeta::destroy()
{
  auto& rd = Terra::get().getDevice();
  for (auto [op, shader] : shaders)
    rd.destroy(shader);
  shaders.clear();
};

Node::Node(NodeMeta const& nm) : meta(&nm)
{
  parameters.resize(nm.parameterDef.size());
  for (uint32_t i = 0; i < parameters.size(); ++i)
    parameters[i] = nm.parameterDef[i].getDefault();
  name = nm.name;
}

dshandle Node::clone(uint32_t)
{
  dshandle ret  = get().createNode(*meta);
  auto&    node = get().get<Node>(ret);
  // node.       = *this;
  assert(node.getSelf() == ret);
  assert(node.meta == meta);

  node.name = this->name;
  for (uint32_t i = 0; i < parameters.size(); ++i)
  {
    if (std::holds_alternative<Source>(parameters[i]))
      node.setValue(i, std::get<Source>(parameters[i]));
    else
      node.setValue(i, std::get<ScalarValue>(parameters[i]));
  }

  return ret;
}

Node::~Node()
{  
  Terra& main = Terra::get();
  if (!meta)
    return;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (std::holds_alternative<Source>(parameters[i]))
    {
      auto h = std::get<Source>(parameters[i]);
      if (DataSource::isValid(h.source))
      {
        auto& src = main.get<DataSource>(h.source);
        src.remove(self);
      }
    }
  }

  self = {};
}

void Node::toDataStreamImpl(std::vector<uint8_t>& dataStream) const
{
  if (!meta)
  {
    terra::addToDataStream(dataStream, false);
    return;
  }

  terra::addToDataStream(dataStream, true);
  terra::addToDataStream(dataStream, meta->id);
  for (auto const& p : parameters)
  {
    uint8_t type = (uint8_t)p.index();
    addToDataStream(dataStream, type);
    std::visit(overloaded{[&dataStream](ScalarValue const& arg)
                          {
                            addToDataStream(dataStream, arg.ivalue2);
                          },
                          [&dataStream](Source const& arg)
                          {
                            addToDataStream(dataStream, arg.source.reserved);
                            addToDataStream(dataStream, arg.secondary);
                          }},
               p);
  }
}

bool Node::fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  auto& main = Terra::get();
  auto& rd   = main.getDevice();

  bool hasMeta = false;
  bool result  = true;
  result &= terra::getFromDataStream(dataStream, serialIdx, hasMeta);

  if (hasMeta)
  {
    std::string name;
    result &= terra::getFromDataStream(dataStream, serialIdx, name);
    meta = main.getNodeMeta(name);
  }

  if (!meta)
    return false;

  parameters.reserve(meta->parameterDef.size());
  uint64_t option = 0;
  for (size_t i = 0; i < meta->parameterDef.size() && result; ++i)
  {
    uint8_t type = 0;
    if (!terra::getFromDataStream(dataStream, serialIdx, type))
      return false;
    if (type == 0)
    {
      ScalarValue sv = {};
      result &= terra::getFromDataStream(dataStream, serialIdx, sv);
      parameters.emplace_back(sv);
    }
    else
    {
      Source sv = {};
      result &= terra::getFromDataStream(dataStream, serialIdx, sv.source.reserved);
      result &= terra::getFromDataStream(dataStream, serialIdx, sv.secondary);
      parameters.emplace_back(sv);
    }
  }

  return true;
}

void Node::sourceDeleted(dshandle src)
{
  for (auto& p : parameters)
  {
    if (std::holds_alternative<Source>(p) && std::get<Source>(p).source == src)
    {
      std::get<Source>(p).source = {};
    }
  }
  markValueChanged();
}

void Node::setValueModified(uint32_t i)
{
  Terra& main = Terra::get();
  if (!meta)
    return;
  propagate(Event::eValueModified);
  markValueChanged();
}

bool Node::setValue(uint32_t i, ScalarValue value)
{
  Terra& main = Terra::get();
  bool   ex   = false;
  if (!meta)
    return ex;
  if (!meta->parameterDef[i].canBeScalar())
    return ex;
  ex = true;
  if (std::holds_alternative<Source>(parameters[i]))
  {
    auto h = std::get<Source>(parameters[i]);
    if (DataSource::isValid(h.source))
      get().get<DataSource>(h.source).remove(self);
  }
  parameters[i] = value;
  markValueChanged();
  return ex;
}

bool Node::setValue(uint32_t i, Source value)
{
  return setParamSource(i, value);
}

void Node::resetValue(uint32_t i) 
{
  setValue(i, paramMeta(i).getDefault());
}

Node::exchange Node::setParamSourceImpl(uint32_t i, Source value)
{
  Terra&   main = Terra::get();
  exchange ex   = exchange(dshandle{}, false);
  if (!meta)
    return ex;
  if (!meta->parameterDef[i].canBeSource())
    return ex;
  if (DataSource::isValid(value.source))
  {
    auto& dsh = get().get<DataSource>(value.source);
    if (!DataFormat::isCompatible(meta->parameterDef[i].format, dsh.getFormat()))
      return ex;
  }

  ex.second = true;
  if (std::holds_alternative<Source>(parameters[i]))
  {
    ex.first = std::get<Source>(parameters[i]).source;
  }
  parameters[i] = value;
  markValueChanged();
  return ex;
}

int32_t Node::incomingEdges() const
{
  int32_t nb = 0;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (meta->parameterDef[i].format.type == DataType::eBuffer)
    {
      auto node = std::get<DataSource>(parameters[i]).node;
      if (node && isValid(node))
      {
        nb++;
      }
    }
  }
  return nb;
}

void Node::markValueChanged()
{
  for (auto& t : tasks)
    t.second.valueChanged = true;
}

bool Node::alreadyComputed(Pipeline const& pipe) 
{
  auto& task = tasks[pipe.taskId()];
  return !task.valueChanged && (task.iteration == pipe.getIteration() || task.iteration > maxIteration) &&
         (task.params == pipe.params());
}
bool Node::ensure(Pipeline& pipe)
{
  auto& task = tasks[pipe.taskId()];
  if (alreadyComputed(pipe))
    return true;
  if (!isEnabled(pipe))
    return true;

  Options     opt = 0;
  auto const& pd  = getMeta().parameterDef;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (!pd[i].modifyOptions(pipe, parameters[i], opt))
      return false;
  }
  // - Options
  task.shader = getMeta().getShaderGLSL(opt);
  if (!task.shader)
    return false;
  if (getMeta().ensure)
  {
    if (Result::eFinished != getMeta().ensure(*this, pipe))
      return false;
  }
  // - Output/EnvParams
  auto const& params    = pipe.params();
  auto        iteration = pipe.getIteration();
  uint32_t    scale     = 1;
  if (meta->outputDownscale)
    scale = iteration * ((uint32_t)std::popcount(meta->outputDownscale - 1));
  else if (meta->outputUpscale)
    scale = iteration * ((uint32_t)std::popcount(meta->outputUpscale - 1));
  uint32_t workGroupSize = get().getWorkGroupSize();
  task.params            = params;
  task.iteration         = iteration;
  bool transient         = !meta->cacheResults;
  if (hasTextureOutput())
  {
    auto width  = params.tileSize[0];
    auto height = params.tileSize[1];
    if (meta->outputDownscale > 1)
    {
      width >>= scale;
      height >>= scale;
    }
    else if (meta->outputUpscale > 1)
    {
      width <<= scale;
      height <<= scale;
    }
    task.outputX            = (width + (workGroupSize - 1)) / workGroupSize;
    task.outputY            = (height + (workGroupSize - 1)) / workGroupSize;
    task.outputId           = pipe.declImage(task.outputId, width, height, meta->imageFormat, transient);
    task.params.tileSize[0] = width;
    task.params.tileSize[1] = height;
  }
  else
  {
    // two more than requested
    auto divisor = (1 << scale);
    auto size    = (params.tileSize[0] + 2) * (params.tileSize[1] + 2);
    size         = ((size + (divisor - 1)) / divisor) * size;
    if (meta->outputDownscale > 1)
    {
      task.params.tileSize[0] >>= (scale >> 1);
      task.params.tileSize[1] >>= (scale >> 1);
      size >>= scale;
    }
    else if (meta->outputUpscale > 1)
    {
      task.params.tileSize[0] <<= (scale >> 1);
      task.params.tileSize[1] <<= (scale >> 1);
      size <<= scale;
    }
    // Recompute size divided by 4 floats we compute at a time
    size                        = ((size + 3) / 4);
    task.outputX                = (size + (workGroupSize - 1)) / workGroupSize;
    task.outputY                = 1;
    task.outputId               = pipe.declBuffer(task.outputId, task.outputX * workGroupSize * 16, transient);
    task.params.bufferArraySize = size / 4;
  }
  task.descriptorSet = terra::get().getDevice().createDescriptorSet(meta->descriptorSetLayout);
  pipe.getUbo().setSize((uint32_t)meta->uboSize);
  // - Execute
  task.valueChanged = false;
  pipe.enqueue(self);
  return true;
}

void Node::deleteTaskData(uint32_t taskId)
{
  terra::get().getDevice().destroy(tasks[taskId].descriptorSet);
  tasks.erase(taskId);
}

Result Node::run( Pipeline& pipeline)
{
  if (meta->run)
    return meta->run(*this, pipeline);

  auto& task          = tasks[pipeline.taskId()];
  auto& main          = terra::get();
  auto& parameterDefs = meta->parameterDef;
  auto& ubo           = pipeline.getUbo();
  auto  uboData       = ubo.map(0, meta->uboSize);
  auto  taskId        = pipeline.taskId();
  std::memcpy(uboData, &tasks[taskId].params, sizeof(EnvParams));
  std::vector<GfxDescriptorSet::rhandle> handles((size_t)meta->nbDescriptors);
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    auto& pdef = parameterDefs[i];
    auto& pval = parameters[i];
    switch (pdef.format.type)
    {
    case DataType::eInt2:
    {
      auto val = std::get<ScalarValue>(pval).ivalue2;
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(ivec2));
    }
    break;
    case DataType::eInt:
    {
      auto val = std::get<ScalarValue>(pval).ivalue;
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(int));
    }
    break;
    case DataType::eFloat2:
    {
      auto val = std::get<ScalarValue>(pval).value2;
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(vec2));
    }
    break;
    case DataType::eFloat:
    {
      auto val = std::get<ScalarValue>(pval).value;
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(float));
    }
    break;
    case DataType::eImage:
    case DataType::eBuffer:
    case DataType::eCurveData:
      if (std::holds_alternative<Source>(pval))
      {

        auto& h = std::get<Source>(pval);
        if (DataSource::isValid(h.source))
        {
          auto& val = get().get<DataSource>(h.source);
          val.fillDescriptor(pipeline, handles[pdef.descriptorIndex], uboData + (size_t)pdef.uboOffset);
          handles[pdef.descriptorIndex].second = h.secondary;
        }
      }
      else
      {
        glsl::fillScalarDisabled(pipeline, pdef, std::get<ScalarValue>(pval), uboData + (size_t)pdef.uboOffset);
      }
      break;
    }    
  }
  ubo.unmap();
  handles[meta->constantsDescriptorIdx].first = ubo.get();
  handles[meta->outputDescriptorIdx].first    = meta->hasTextureOutput
                                                  ? (uint32_t)pipeline.getOutputImage(dshandle(self))
                                                  : (uint32_t)pipeline.getOutputBuffer(dshandle(self));
  main.getDevice().updateDescriptorSet(tasks[taskId].descriptorSet, handles);
  main.getDevice().barrier(GfxBarrierFlags::fFullBarrier);
  main.getDevice().dispatchCompute(tasks[taskId].shader, tasks[taskId].descriptorSet, tasks[taskId].outputX,
                                   tasks[taskId].outputY);

  return (task.iteration < maxIteration) ? Result::eContinue : Result::eFinished;
}

*/

} // namespace terra