
#include "Node.h"
#include "Logger.h"
#include "Pipeline.h"
#include "ShaderBuilder.h"
#include "Terra.h"
#include <charconv>
#include <numeric>

namespace tmpl
{
#include "glsl/buffer.glsl"
#include "glsl/curve430.glsl"
#include "glsl/image.glsl"
#include "glsl/texture.glsl"
} // namespace tmpl

namespace terra
{

uint32 subtypeSize(DataType type)
{
  switch (type)
  {
  case DataType::eFloat:
  case DataType::eInt:
    return 4;
  case DataType::eInt2:
  case DataType::eFloat2:
    return 8;
  case DataType::eBool:
    return 1;
  }
  return 0;
}

std::string_view bufferReadType(DataType type)
{
  switch (type)
  {
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
  }
  return "invalid";
}

std::string_view bufferWriteType(DataType type)
{
  switch (type)
  {
  case DataType::eInt:
    return "ivec4";
  case DataType::eInt2:
    return "imat2x4";
  case DataType::eFloat:
    return "vec4";
  case DataType::eFloat2:
    return "vec2x4";
  case DataType::eBool:
    return "bvec4";
  }
  return "invalid";
}

std::string_view typeToString(DataType type) 
{
  switch (type)
  {
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
  case DataType::eDataSource:
    return "source";
  case DataType::eCurveData:
    return "curve";
  }
  return "invalid";
}

DataType stringToType(std::string_view stype)
{
  DataType type = DataType::eInvalid;
  if (stype == "int")
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
  else if (stype == "source")
    type = DataType::eDataSource;
  else if (stype == "curve")
    type = DataType::eCurveData;
  return type;
}

Parameter ParameterMeta::getDefault() const 
{
  switch (format.type)
  {
  case DataType::eBool:
    return (bool)(values[ValueType::eDefault].ival != 0);
  case DataType::eCurveData:
    return std::make_shared<CurveData>();
  case DataType::eDataSource:
    return DataSource();
  case DataType::eFloat:
    return values[ValueType::eDefault].fval;
  case DataType::eFloat2:
    return vec2{values[ValueType::eDefault].fval, values[ValueType::eDefault].fval};
  case DataType::eImage:
    return ImageSource();
  case DataType::eInt:
    return values[ValueType::eDefault].ival;
  case DataType::eInt2:
    return ivec2{values[ValueType::eDefault].ival, values[ValueType::eDefault].ival};
  default:
    return Parameter();
  }
}

void      ParameterMeta::setTypeFromString(std::string_view stype)
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
  case DataType::eDataSource:
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

bool ParameterMeta::affectsOptions() const
{
  switch (format.type)
  {
  case DataType::eImage:
  case DataType::eDataSource:
  case DataType::eBool:
    return true;
  }
  return false;
}

void ParameterMeta::modifyOptions(Parameter const& p, Options& option) const
{
  switch (format.type)
  {
  case DataType::eImage:
  {
    auto& image = std::get<ImageSource>(p);
    if (image.isValidSource())
      option |= 1ull << (Options)optionIndex[0];
    if (image.tileConstraintMax[0] - image.tileConstraintMin[0] > 0 &&
        image.tileConstraintMax[1] - image.tileConstraintMin[1])
      option |= 1ull << (Options)optionIndex[1];
    break;
  }
  case DataType::eDataSource:
  {
    auto& data = std::get<DataSource>(p);
    if (data.node)
      option |= 1ull << (Options)optionIndex[0];
    break;
  }
  case DataType::eBool:
    if (std::get<bool>(p))
      option |= 1ull << (Options)optionIndex[0];
    break;
  }
}

std::string NodeMeta::writeTextureSamplerGLSL(std::string_view name)
{
  return std::format(tmpl::gs_textureLoad, name);
}

std::string NodeMeta::writeDataSamplerGLSL(RenderDevice::Caps const& caps, std::string_view name)
{
  return std::format(tmpl::gs_bufferLoad, name);
}

std::string NodeMeta::writeCurveSamplerGLSL(RenderDevice::Caps const& caps, std::string_view name)
{
  return std::format(tmpl::gs_curve430, name);
}

std::string NodeMeta::writeImageStoreGLSL(std::string_view name)
{
  return std::format(tmpl::gs_imageStore, name);
}

std::string NodeMeta::writeBufferStoreGLSL(std::string_view name)
{
  return std::format(tmpl::gs_bufferStore, name);
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
  shaderBuilder->append(std::format(
    "#define Binding_Node {}\n"
    "#define WorkGroupSize {}\n", 
    "#define output_t {}\n", 
    nodeContent.function, get().getWorkGroupSize(),
                                    bufferWriteType(format.scalarSubType)));
  shaderBuilder->append(commonContent.typesAndConstants);
  shaderBuilder->append(commonContent.fixedResources);
  shaderBuilder->append(commonContent.utilityFunctions);

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
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case DataType::eFloat:
      nodeParams += "  float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case DataType::eInt2:
      nodeParams += "  ivec2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case DataType::eFloat2:
      nodeParams += "  vec2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case DataType::eBool:
      t.optionIndex[0] = (int)options.size();
      options.push_back(t.name + "_Enabled");
      generated += std::format("const bool {0} = {0}_Enabled;\n", t.name);
      break;
    case DataType::eDataSource:
      nodeParams += std::format("  {0}_t {0};\n", t.name);
      nodeParams += t.name;
      t.uboOffset = paramOffsets;
      paramOffsets += subtypeSize(t.format.scalarSubType);
      break;
    case DataType::eImage:
      nodeParams += "  float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  float uv_scale_";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  uint2 tile_vert_min_";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  uint2 tile_vert_max_";
      nodeParams += t.name;
      nodeParams += ";\n";

      t.uboOffset = paramOffsets;
      paramOffsets += 24;
      break;
    }
  }
  // deal with buffers and textures
  for (auto o : declOrder)
  {
    auto&                      t = parameterDef[o];
    ShaderBuilder::BindingInfo bi;
    switch (t.format.type)
    {
    case DataType::eImage:
      t.optionIndex[0] = (int)options.size();
      options.push_back("Has_" + t.name);
      t.optionIndex[1] = (int)options.size();
      options.push_back("IsTileConstrainted_" + t.name);
      generated += std::format("const bool has_{0} = Has_{0};\n", t.name);
      generated += std::format("const bool is_tile_constrained_{0} = IsTileConstrained_{0};\n", t.name);
      bi = shaderBuilder->declTexture(t.name);
      generated += bi.content;
      generated += ";\n";
      generated += writeTextureSamplerGLSL(t.name);
      t.descriptorIndex = (int)descriptorSetBindings.size();
      descriptorSetBindings.emplace_back(bi.descriptor);

      break;
    case DataType::eDataSource:
      t.optionIndex[0] = (int)options.size();
      options.push_back("Has_" + t.name);
      generated += std::format("const bool has_{0} = Has_{0};\n", t.name);
      bi = shaderBuilder->declBuffer("U", t.name, Access::eReadonly);
      generated += bi.content;
      generated += std::format("{{ {0}_t data[]; }}", t.name);
      generated += t.name;
      generated += ";\n";
      generated += writeDataSamplerGLSL(caps, t.name);
      shaderBuilder->append(std::format("#define {0}_t {1}\n"
                                        "#define {0}_t4 {2}\n",
                                        t.name, bufferReadType(t.format.scalarSubType), bufferWriteType(t.format.scalarSubType)));
      t.descriptorIndex = (int)descriptorSetBindings.size();
      descriptorSetBindings.emplace_back(bi.descriptor);

      break;
    case DataType::eCurveData:
      bi = shaderBuilder->declBuffer("U", t.name, Access::eReadonly);
      generated += bi.content;
      generated += "{ float c0; uint npoints; float data[]; }";
      generated += t.name;
      generated += ";\n";
      generated += writeCurveSamplerGLSL(caps, t.name);
      t.descriptorIndex = (int)descriptorSetBindings.size();
      descriptorSetBindings.emplace_back(bi.descriptor);

      break;
    }
  }

  
  if (hasTextureOutput)
  {
    shaderBuilder->append("#define HasTextureOutput 1\n");
    auto bi = shaderBuilder->declImage("output", ImageFormat::eFloat, Access::eWriteonly);
    generated += bi.content;
    generated += ";\n";
    generated += writeImageStoreGLSL("output");
    outputDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(bi.descriptor);
  }
  else
  {
    auto bi = shaderBuilder->declBuffer("U", "Output", Access::eWriteonly);
    generated += bi.content;
    generated += "{ output_t data[]; }";    
    generated += writeBufferStoreGLSL("output");
    outputDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(bi.descriptor);
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
  shaderBuilder->append(generated);
  shaderBuilder->append("\n");
  {
    auto bi = shaderBuilder->declConstants("U", "Constants");
    shaderBuilder->append(bi.content);
    shaderBuilder->append("{ NodeParams params; };\n");
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

hnode Node::clone(uint32_t)
{
  hnode ret  = get().createNode(*meta);
  auto& node = get().getNode(ret);
  node       = *this;
  node.tasks.clear();
  return ret;
}

Node::~Node()
{
  propagate(NodeEvent::eValueModified);
  Terra& main = Terra::get();
  if (!meta)
    return;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (meta->parameterDef[i].format.type == DataType::eDataSource)
    {
      auto oldNode = std::get<DataSource>(parameters[i]).node;
      if (oldNode && main.isValid(oldNode))
        main.getNode(oldNode).remove(id);
    }
    else if (meta->parameterDef[i].format.type == DataType::eImage)
    {
      auto& oldNode = std::get<ImageSource>(parameters[i]).source;
      if (std::holds_alternative<ImageDataIdx>(oldNode) && std::get<ImageDataIdx>(oldNode))
        main.getImage(std::get<ImageDataIdx>(oldNode)).remove(id);
      else if (std::holds_alternative<hnode>(oldNode) && isValid(std::get<hnode>(oldNode)))
        main.getNode(std::get<hnode>(oldNode)).remove(id);
    }
  }
}

void Node::toDataStream(std::vector<uint8_t>& dataStream) const
{
  terra::addToDataStream(dataStream, id.reserved);
  if (!meta)
  {
    terra::addToDataStream(dataStream, false);
    return;
  }

  terra::addToDataStream(dataStream, true);
  terra::addToDataStream(dataStream, meta->id);
  for (auto const& p : parameters)
  {
    std::visit(overloaded{[](std::monostate arg) {},
                          [&dataStream](auto arg)
                          {
                            addToDataStream(dataStream, arg);
                          },
                          [&dataStream](ImageSource const& arg)
                          {
                            arg.toDataStream(dataStream);
                          },
                          [&dataStream](DataSource const& arg)
                          {
                            arg.toDataStream(dataStream);
                          },
                          [&dataStream](CurveDataPtr const& arg)
                          {
                            arg->toDataStream(dataStream);
                          }},
               p);
  }
}

bool Node::fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  auto& main = Terra::get();
  auto& rd   = main.getDevice();

  bool hasMeta = false;

  terra::getFromDataStream(dataStream, serialIdx, id.reserved);
  terra::getFromDataStream(dataStream, serialIdx, hasMeta);

  if (hasMeta)
  {
    std::string name;
    terra::getFromDataStream(dataStream, serialIdx, name);
    meta = main.getNodeMeta(name);
  }

  if (!meta)
    return false;

  parameters.resize(meta->parameterDef.size());
  uint64_t option = 0;
  for (size_t i = 0; i < parameters.size(); ++i)
  {
    auto&   p = parameters[i];
    auto&   m = meta->parameterDef[i];
    uint8_t type;
    if (!terra::getFromDataStream(dataStream, serialIdx, type))
      return false;
    switch (m.format.type)
    {
    case DataType::eInt: // int
    {
      int value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case DataType::eFloat: // int
    {
      float value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case DataType::eInt2: // int
    {
      ivec2 value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case DataType::eFloat2: // int
    {
      vec2 value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case DataType::eBool: // int
    {
      bool value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case DataType::eDataSource: // int
    {
      DataSource value;
      if (!value.fromDataStream(dataStream, serialIdx))
        return false;
      p = std::move(value);
    }
    break;
    case DataType::eImage: // int
    {
      ImageSource value;
      if (!value.fromDataStream(dataStream, serialIdx))
        return false;
      p = std::move(value);
    }
    break;
    case DataType::eCurveData: // int
    {
      CurveDataPtr value = std::make_shared<CurveData>();
      if (!value->fromDataStream(dataStream, serialIdx))
        return false;
      p = std::move(value);
    }
    break;
    }
    m.modifyOptions(p, option);
  }

  valueChanged  = true;
  optionChanged = false;
  shader        = meta->getShaderGLSL(option);
  return true;
}

void Node::prepare()
{
  Terra& main = Terra::get();
  if (valueChanged)
  {
    propagate(NodeEvent::eValueModified);
    for (auto& p : parameters)
    {
      if (std::holds_alternative<ImageSource>(p))
      {
        auto& img = std::get<ImageSource>(p);
        if (std::holds_alternative<ImageDataIdx>(img.source))
          main.getImage(std::get<ImageDataIdx>(img.source)).ensure();
      }
      else if (std::holds_alternative<CurveDataPtr>(p))
      {
        auto& crv = std::get<CurveDataPtr>(p);
        crv->ensure();
      }
    }
  }

  if (optionChanged)
  {
    Options option = 0;
    for (uint32_t i = 0; i < parameters.size(); ++i)
    {
      auto const& p   = parameters[i];
      auto const& def = meta->parameterDef[i];
      def.modifyOptions(p, option);
    }
    shader = meta->getShaderGLSL(option);

    optionChanged = false;
    valueChanged  = true;
  }
}

bool Node::isInputCompatible(uint32_t i, DataFormat const& f) 
{
  if (!meta)
    return false;
  return (meta->parameterDef[i].format == f);
}

void Node::setValue(uint32_t i, Parameter&& value)
{
  Terra& main = Terra::get();
  if (!meta)
    return;
  assert(parameters[i].index() == value.index());
  if (meta->parameterDef[i].format.type == DataType::eDataSource)
  {
    auto oldNode = std::get<DataSource>(parameters[i]).node;
    if (oldNode && isValid(oldNode))
      main.getNode(oldNode).remove(id);
    auto newNode = std::get<DataSource>(value).node;
    if (newNode)
      main.getNode(newNode).add(id);
  }
  else if (meta->parameterDef[i].format.type == DataType::eImage)
  {
    auto& oldNode = std::get<ImageSource>(parameters[i]).source;
    if (std::holds_alternative<ImageDataIdx>(oldNode) && std::get<ImageDataIdx>(oldNode))
      main.getImage(std::get<ImageDataIdx>(oldNode)).remove(id);
    else if (std::holds_alternative<hnode>(oldNode) && isValid(std::get<hnode>(oldNode)))
      main.getNode(std::get<hnode>(oldNode)).remove(id);

    auto& newNode = std::get<ImageSource>(value).source;

    if (std::holds_alternative<ImageDataIdx>(newNode) && std::get<ImageDataIdx>(newNode))
      main.getImage(std::get<ImageDataIdx>(newNode)).add(id);
    else if (std::holds_alternative<hnode>(newNode) && isValid(std::get<hnode>(newNode)))
      main.getNode(std::get<hnode>(newNode)).add(id);
  }
  parameters[i] = std::move(value);
  markValueChanged();
  if (meta->parameterDef[i].affectsOptions())
    markOptionChanged();
}

bool Node::isValid(hnode idx)
{
  return Terra::get().isValid(idx);
}

int32_t Node::incomingEdges() const
{
  int32_t nb = 0;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (meta->parameterDef[i].format.type == DataType::eDataSource)
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

bool Node::isReadyToExecute(uint32_t taskId)
{
  return (taskId < tasks.size() && tasks[taskId].ready);
}

void Node::enqueue(uint32_t taskId, uint32_t iteration, Pipeline& pipeline)
{
  if (taskId >= tasks.size())
    tasks.resize(taskId + 1);
  auto const& params = pipeline.params();
  uint32_t    scale  = 1;
  if (meta->outputDownscale)
    scale = iteration * ((uint32_t)std::popcount(meta->outputDownscale - 1));
  else if (meta->outputUpscale)
    scale = iteration * ((uint32_t)std::popcount(meta->outputUpscale - 1));
  uint32_t workGroupSize = get().getWorkGroupSize();
  tasks[taskId].params   = params;
  if (hasTextureOutput())
  {
    auto width  = params.size[0];
    auto height = params.size[1];
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
    tasks[taskId].outputX        = (width + (workGroupSize - 1)) / workGroupSize;
    tasks[taskId].outputY        = (height + (workGroupSize - 1)) / workGroupSize;
    tasks[taskId].outputId       = pipeline.declImage(width, height, meta->imageFormat);
    tasks[taskId].params.size[0] = width;
    tasks[taskId].params.size[1] = height;
  }
  else
  {
    // two more than requested
    auto divisor = (1 << scale);
    auto size = (params.size[0] + 2) * (params.size[1] + 2);
    size         = ((size + (divisor - 1)) / divisor) * size;
    if (meta->outputDownscale > 1)
    {
      tasks[taskId].params.size[0] >>= (scale >> 1);
      tasks[taskId].params.size[1] >>= (scale >> 1);
      size >>= scale;
    }
    else if (meta->outputUpscale > 1)
    {
      tasks[taskId].params.size[0] <<= (scale >> 1);
      tasks[taskId].params.size[1] <<= (scale >> 1);
      size <<= scale;
    }
    // Recompute size divided by 4 floats we compute at a time
    size = ((size + 3) / 4);
    tasks[taskId].outputX                = (size + (workGroupSize - 1)) / workGroupSize;
    tasks[taskId].outputY                = 1;
    tasks[taskId].outputId               = pipeline.declBuffer(tasks[taskId].outputX * workGroupSize * 16);
    tasks[taskId].params.bufferArraySize = size / 4;
  }
  tasks[taskId].descriptorSet = terra::get().getDevice().createDescriptorSet(meta->descriptorSetLayout);
  pipeline.getUbo().setSize((uint32_t)meta->uboSize);
}

void Node::deleteTaskData(uint32_t taskId)
{
  tasks[taskId].ready = false;
  terra::get().getDevice().destroy(tasks[taskId].descriptorSet);
  tasks[taskId].descriptorSet = {};
}

void Node::run(uint32_t taskId, Pipeline& pipeline)
{
  auto& main          = terra::get();
  auto& parameterDefs = meta->parameterDef;
  auto& ubo           = pipeline.getUbo();
  auto  uboData       = ubo.map(0, meta->uboSize);
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
      auto val = std::get<ivec2>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(ivec2));
    }
    break;
    case DataType::eInt:
    {
      auto val = std::get<int>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(int));
    }
    break;
    case DataType::eFloat2:
    {
      auto val = std::get<vec2>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(vec2));
    }
    break;
    case DataType::eFloat:
    {
      auto val = std::get<float>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(float));
    }
    break;
    case DataType::eImage:
    {
      auto& val = std::get<ImageSource>(pval);

      *(float*)(uboData + (size_t)pdef.uboOffset)     = val.defaultValue;
      *(float*)(uboData + (size_t)pdef.uboOffset + 4) = val.uvScale;
      *(ivec2*)(uboData + (size_t)pdef.uboOffset + 8)  = val.tileConstraintMin;
      *(ivec2*)(uboData + (size_t)pdef.uboOffset + 16) = val.tileConstraintMax;

      if (std::holds_alternative<ImageDataIdx>(val.source))
        handles[pdef.descriptorIndex].first = main.getImage(std::get<ImageDataIdx>(val.source)).handle;
      else if (std::holds_alternative<hnode>(val.source))
        handles[pdef.descriptorIndex].first = pipeline.getOutputImage(std::get<hnode>(val.source));
      handles[pdef.descriptorIndex].second = val.sampler;
    }
    break;
    case DataType::eCurveData:
    {
      auto& val                           = std::get<CurveDataPtr>(pval);
      handles[pdef.descriptorIndex].first = val->handle;
    }
    break;
    case DataType::eDataSource:
    {
      auto& val                                   = std::get<DataSource>(pval);
      *(float*)(uboData + (size_t)pdef.uboOffset) = val.constValue;
      handles[pdef.descriptorIndex].first         = pipeline.getOutputBuffer(val.node);
    }
    break;
    }
  }
  ubo.unmap();
  handles[meta->constantsDescriptorIdx].first = ubo.get();
  handles[meta->outputDescriptorIdx].first    = meta->hasTextureOutput ? (uint32_t)pipeline.getOutputImage(hnode(id))
                                                                       : (uint32_t)pipeline.getOutputBuffer(hnode(id));
  main.getDevice().updateDescriptorSet(tasks[taskId].descriptorSet, handles);
  main.getDevice().barrier(GfxBarrierFlags::fFullBarrier);
  main.getDevice().dispatchCompute(shader, tasks[taskId].descriptorSet, tasks[taskId].outputX, tasks[taskId].outputY);
}

} // namespace terra