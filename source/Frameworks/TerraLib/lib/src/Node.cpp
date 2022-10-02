
#include "Node.h"
#include "Pipeline.h"
#include "ShaderBuilder.h"
#include "Terra.h"
#include <charconv>
#include <numeric>

namespace terra
{

auto stringToType(std::string_view stype)
{
  ParameterType type = ParameterType::eInvalid;
  if (stype == "int")
    type = ParameterType::eInt;
  else if (stype == "float")
    type = ParameterType::eFloat;
  else if (stype == "int2")
    type = ParameterType::eInt2;
  else if (stype == "float2")
    type = ParameterType::eFloat2;
  else if (stype == "bool")
    type = ParameterType::eBool;
  else if (stype == "image")
    type = ParameterType::eImage;
  else if (stype == "source")
    type = ParameterType::eDataSource;
  else if (stype == "curve")
    type = ParameterType::eCurveData;
  return type;
}

void ParameterMeta::setTypeFromString(std::string_view stype)
{
  type = stringToType(stype);
}

void ParameterMeta::setValueFromString(ValueType valType, std::string_view value)
{
  auto setter = [this, valType](auto value)
  {
    values[valType] = ParamValue(value);
  };
  int   ivalue = 0;
  float fvalue = 0;
  switch (type)
  {
  case ParameterType::eBool:
  case ParameterType::eInt2:
  case ParameterType::eInt:
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
  case ParameterType::eCurveData:
  case ParameterType::eImage:
  case ParameterType::eDataSource:
  case ParameterType::eFloat:
  case ParameterType::eFloat2:
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
  switch (type)
  {
  case ParameterType::eImage:
  case ParameterType::eDataSource:
  case ParameterType::eBool:
    return true;
  }
  return false;
}

void ParameterMeta::modifyOptions(Parameter const& p, Options& option) const
{
  switch (type)
  {
  case ParameterType::eImage:
  {
    auto& image = std::get<ImageSource>(p);
    if (image.isValidSource())
      option |= 1ull << (Options)optionIndex[0];
    if (image.tileConstraintMax[0] - image.tileConstraintMin[0] > 0 &&
        image.tileConstraintMax[1] - image.tileConstraintMin[1])
      option |= 1ull << (Options)optionIndex[1];
    break;
  }
  case ParameterType::eDataSource:
  {
    auto& data = std::get<DataSource>(p);
    if (data.node)
      option |= 1ull << (Options)optionIndex[0];
    break;
  }
  case ParameterType::eBool:
    if (std::get<bool>(p))
      option |= 1ull << (Options)optionIndex[0];
    break;
  }
}

std::string NodeMeta::writeTextureSamplerGLSL(std::string_view name)
{

  constexpr std::string_view glob = R"_(
"                                                                                                            
"float sample_{0}(float u, float v, int x, int y, NodeParams np)                                             
"{{                                                                                                           
"  if (has_{0})                                                                                              
"  {{                                                                                                         
"    if (is_tile_constrained_{0})                                                                            
"    {{                                                                                                       
"      if(!is_within_tile(x, y, np.uniforms.tile_vert_min_{0}, np.uniforms.tile_vert_max_{0}))               
"        return np.uniforms.{0};                                                                             
"    }}                                                                                                       
"    return texture({0}, vec2(u.x * np.uniforms.uv_scale_{0}, v.x * np.uniforms.uv_scale_{0})).r;            
"  }}                                                                                                         
"  else                                                                                                      
"  {{                                                                                                         
"    return np.uniforms.{0};                                                                                 
"  }}                                                                                                         
"}}                                                                                                           
"                                                                                                            
"vec4 sample_{0}(vec4 u, vec4 v, int4 x, int4 y, NodeParams np)                                              
"{{                                                                                                           
"  return vec4(sample_{0}(u.x, v.x, x.x, y.x, np),                                                           
"              sample_{0}(u.y, v.y, x.y, y.y, np),                                                           
"              sample_{0}(u.z, v.z, x.z, y.z, np),                                                           
"              sample_{0}(u.w, v.w, x.w, y.w, np));                                                          
"}}                                                                                                           
"                                                                                                            
)_";

  return std::format(glob, name);
}

std::string NodeMeta::writeDataSamplerGLSL(std::string_view name)
{
  constexpr std::string_view glob = R"_(
 
vec4 sample_{0}(int4 x, int4 y, NodeParams np)
{{ 
  if (has_{0})
  {{
    int4 id = pixel_id(x, y, np);
    return vec4(
      {0}.data[id.x],
      {0}.data[id.y],
      {0}.data[id.z],
      {0}.data[id.w]);
  }}
  else
  {{
    return vec4(np.uniforms.{0});
  }}
}}

float sample_{0}(int x, int y, NodeParams np)
{{ 
  if (has_{0})
  {{
    int id = pixel_id(x, y, np);
    return {0}.data[id];
  }}
  else
  {{
    return np.uniforms.{0};
  }}
}}

)_";

  return std::format(glob, name);
}

std::string NodeMeta::writeCurveSamplerGLSL(std::string_view name)
{
  constexpr std::string_view glob = R"_(

uint closest_{0}(float x, NodeParams np)
{{
  for(uint i = 1; i < {0}.npoints; ++i)
  {{
    if(x < {0}.data[i])
      return i-1;
  }}
  return {0}.npoints-1;
}}

float sample_{0}(float x, NodeParams np)
{{
  uint n   = {0}.npoints;
  uint idx = closest_{0}( x );
  const uint sx = 0;
  const uint sy = n;
  const uint sb = n*2;
  const uint sc = n*3;
  const uint sd = n*4;

  float h = x - {0}.data[idx];
  float interpol = 0.0;
  if( x < {0}.data[0] )
  {{
      // extrapolation to the left
      interpol = ( {0}.c0 * h + {0}.data[sb] ) * h + {0}.data[sy];
  }}
  else if( x > {0}.data[n - 1] )
  {{
      // extrapolation to the right
      interpol = ( {0}.data[sc + n - 1] * h + {0}.data[sb + n - 1] ) * h + {0}.data[sy + n - 1];
  }}
  else
  {{
      // interpolation
      interpol = ( ( {0}.data[sd + idx] * h + {0}.data[sc + idx] ) * h + {0}.data[sb + idx] ) * h + {0}.data[sy + idx];
  }}
  return interpol;
}}

float sample_{0}(float x, float y, NodeParams np)
{{
  return sample_{0}(x, np) + sample_{0}(y, np);
}}

vec4 sample_{0}(vec4 x, NodeParams np)
{{
  return vec4(sample_{0}(x.x, np), sample_{0}(x.y, np), sample_{0}(x.z, np), sample_{0}(x.w, np)); 
}}

vec4 sample_{0}(vec4 x, vec4 y, NodeParams np)
{{
  return sample_{0}(x, np) + sample_{0}(y, np); 
}}

)_";
  return std::format(glob, name);
}

std::string NodeMeta::writeImageStoreGLSL(std::string_view name)
{
  constexpr std::string_view glob = R"_(

void store_{0}(int4 x, int4 y, float4 value, NodeParams np)
{{
  // int4 id = pixel_id(x, y, np);
  imageStore({0}, ivec2(x.x, y.x), vec4(value.x));
  imageStore({0}, ivec2(x.y, y.y), vec4(value.y));
  imageStore({0}, ivec2(x.z, y.z), vec4(value.z));
  imageStore({0}, ivec2(x.w, y.w), vec4(value.w));
}} 

void store_{0}(int x, int y, float value, NodeParams np)
{{
  // int4 id = pixel_id(x, y, np);
  imageStore({0}, ivec2(x, y), vec4(value));
}} 

)_";
  return std::format(glob, name);
}

std::string NodeMeta::writeBufferStoreGLSL(std::string_view name)
{
  constexpr std::string_view glob = R"_(

void store_{0}(int4 x, int4 y, float4 value, NodeParams np)
{{
  int4 id = pixel_id(x, y, np);
  {0}.data[id.x] = value.x;
  {0}.data[id.y] = value.y;
  {0}.data[id.z] = value.z;
  {0}.data[id.w] = value.w;
}} 

void store_{0}(int x, int y, float value, NodeParams np)
{{
  int id = pixel_id(x, y, np);
  {0}.data[id] = value.x;
}} 

)_";
  return std::format(glob, name);
}

void NodeMeta::buildShaderGLSL()
{
  auto&       main          = Terra::get();
  auto&       rd            = main.getDevice();
  auto const& shaderContent = main.getShaderContent(GfxCompute::Language::eGLSL);
  std::string generated;
  std::string nodeParams;
  auto        shaderBuilder = rd.createShaderBuilder(GfxCompute::Language::eGLSL);
  // generated content
  int32_t paramOffsets = 0;
  auto    declOrder    = std::vector<uint32_t>(parameterDef.size());

  std::iota(declOrder.begin(), declOrder.end(), 0);
  std::sort(declOrder.begin(), declOrder.end(),
            [this](uint32_t first, uint32_t second)
            {
              return parameterDef[first].type < parameterDef[second].type;
            });

  std::vector<GfxDescriptorSetLayout::Descriptor> descriptorSetBindings;
  using DT = GfxDescriptorSetLayout::DescriptorType;

  // print only constants
  for (auto o : declOrder)
  {
    auto& t = parameterDef[o];
    switch (t.type)
    {
    case ParameterType::eInt:
      nodeParams += "  int ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case ParameterType::eFloat:
      nodeParams += "  float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case ParameterType::eInt2:
      nodeParams += "  ivec2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case ParameterType::eFloat2:
      nodeParams += "  vec2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case ParameterType::eBool:
      t.optionIndex[0] = (int)options.size();
      options.push_back(t.name + "_Enabled");
      generated += std::format("const bool {0} = {0}_Enabled;\n", t.name);
      break;
    case ParameterType::eDataSource:
      nodeParams += "  float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.uboOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case ParameterType::eImage:
      nodeParams += "  float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  float uv_scale_";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  int2 tile_vert_min_";
      nodeParams += t.name;
      nodeParams += ";\n";
      nodeParams += "  int2 tile_vert_max_";
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
    switch (t.type)
    {
    case ParameterType::eImage:
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
      descriptorSetBindings.emplace_back(DT::eReadonlyImage, bi.binding);
      break;
    case ParameterType::eDataSource:
      t.optionIndex[0] = (int)options.size();
      options.push_back("Has_" + t.name);
      generated += std::format("const bool has_{0} = Has_{0};\n", t.name);
      bi = shaderBuilder->declBuffer("U", t.name, true);
      generated += bi.content;
      generated += "{ float data[]; }";
      generated += t.name;
      generated += ";\n";
      generated += writeDataSamplerGLSL(t.name);
      t.descriptorIndex = (int)descriptorSetBindings.size();
      descriptorSetBindings.emplace_back(DT::eReadonlyBuffer, bi.binding);
      break;
    case ParameterType::eCurveData:
      bi = shaderBuilder->declBuffer("U", t.name, true);
      generated += bi.content;
      generated += "{ float c0; uint npoints; float data[]; }";
      generated += t.name;
      generated += ";\n";
      generated += writeCurveSamplerGLSL(t.name);
      t.descriptorIndex = (int)descriptorSetBindings.size();
      descriptorSetBindings.emplace_back(DT::eReadonlyBuffer, bi.binding);
      break;
    }
  }

  if (hasTextureOutput)
  {
    auto bi = shaderBuilder->declImage("output");
    generated += bi.content;
    generated += ";\n";
    generated += writeImageStoreGLSL("output");
    outputDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(DT::eImage, bi.binding);
  }
  else
  {
    auto bi = shaderBuilder->declBuffer("U", "Output", false);
    generated += bi.content;
    generated += "{ float data[] };\n";
    generated += writeBufferStoreGLSL("output");
    outputDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(DT::eBuffer, bi.binding);
  }

  uboSize     = paramOffsets + (int)sizeof(EnvParams);
  hasUniforms = (!nodeParams.empty() && paramOffsets != sizeof(EnvParams));
  if (hasUniforms)
  {
    code += "#define NodeUniforms_Enabled 1\n";
    code += "struct NodeUniforms\n{";
    code += nodeParams;
    code += "};\n";
  }
  else
    code += "#define NodeUniforms_Enabled 0\n";

  code = std::move(generated);
  code += "\n";
  {
    auto bi = shaderBuilder->declConstants("U", "Constants");
    code += bi.content;
    code += "{ NodeParams params; };\n";
    constantsDescriptorIdx = (int)descriptorSetBindings.size();
    descriptorSetBindings.emplace_back(DT::eConstants, bi.binding);
  }
  code += "\n";
  if (options.size() >= 64)
  {
    main.logError(std::format("Too many options for : {}", id));
  }
  nbDescriptors       = (int)descriptorSetBindings.size();
  descriptorSetLayout = main.getDevice().createDescriptorSetLayout(descriptorSetBindings);
}

GfxCompute::handle NodeMeta::getShaderGLSL(Options optionBitSet)
{
  auto&       main            = Terra::get();
  auto&       rd              = main.getDevice();
  auto const& shaderFragments = main.getShaderContent(GfxCompute::Language::eGLSL);
  auto        shaderBuilder   = rd.createShaderBuilder(GfxCompute::Language::eGLSL);
  auto        it              = shaders.find(optionBitSet);
  if (it == shaders.end())
  {
    std::vector<std::string_view> content;
    std::string                   defines;
    for (uint64_t i = 0, end = options.size(); i != end; ++i)
    {
      defines += "#define ";
      defines += options[i];
      defines += (optionBitSet & (1ull << i)) ? " 1\n" : " 0\n";
    }
    content.emplace_back(defines);
    content.emplace_back(shaderFragments.preamble);
    content.emplace_back(extensions);
    content.emplace_back(code);
    content.emplace_back(shaderFragments.typesAndConstants);
    content.emplace_back(shaderFragments.fixedResources);
    content.emplace_back(shaderFragments.utilityFunctions);
    content.emplace_back(shaderContent);
    auto shader = rd.createComputeShader(content, GfxCompute::Language::eGLSL);
    if (!shader)
    {
      main.logError("Failed to compile shader.");
      return {};
    }
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

Node::~Node()
{
  propagate(NodeEvent::eValueModified);
  Terra& main = Terra::get();
  if (!meta)
    return;
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    if (meta->parameterDef[i].type == ParameterType::eDataSource)
    {
      auto oldNode = std::get<DataSource>(parameters[i]).node;
      if (oldNode && main.isValid(oldNode))
        main.getNode(oldNode).remove(id);
    }
    else if (meta->parameterDef[i].type == ParameterType::eImage)
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
    switch (m.type)
    {
    case ParameterType::eInt: // int
    {
      int value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case ParameterType::eFloat: // int
    {
      float value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case ParameterType::eInt2: // int
    {
      int2 value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case ParameterType::eFloat2: // int
    {
      float2 value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case ParameterType::eBool: // int
    {
      bool value;
      if (!terra::getFromDataStream(dataStream, serialIdx, value))
        return false;
      p = value;
    }
    break;
    case ParameterType::eDataSource: // int
    {
      DataSource value;
      if (!value.fromDataStream(dataStream, serialIdx))
        return false;
      p = std::move(value);
    }
    break;
    case ParameterType::eImage: // int
    {
      ImageSource value;
      if (!value.fromDataStream(dataStream, serialIdx))
        return false;
      p = std::move(value);
    }
    break;
    case ParameterType::eCurveData: // int
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

void Node::setValue(uint32_t i, Parameter&& value)
{
  Terra& main = Terra::get();
  if (!meta)
    return;
  assert(parameters[i].index() == value.index());
  if (meta->parameterDef[i].type == ParameterType::eDataSource)
  {
    auto oldNode = std::get<DataSource>(parameters[i]).node;
    if (oldNode && isValid(oldNode))
      main.getNode(oldNode).remove(id);
    auto newNode = std::get<DataSource>(value).node;
    if (newNode)
      main.getNode(newNode).add(id);
  }
  else if (meta->parameterDef[i].type == ParameterType::eImage)
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
    if (meta->parameterDef[i].type == ParameterType::eDataSource)
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

  if (hasTextureOutput())
  {
    auto width  = params.bufferSize[0];
    auto height = params.bufferSize[1];
    if (meta->outputDownscale > 1)
    {
      width >>= scale;
      height >>= scale;
    }
    else
    {
      width <<= scale;
      height <<= scale;
    }
    tasks[taskId].outputX  = width;
    tasks[taskId].outputY  = height;
    tasks[taskId].outputId = pipeline.declImage(width, height, meta->imageFormat);
  }
  else
  {
    // two more than requested
    auto size = (params.bufferSize[0] + 2) * (params.bufferSize[1] + 2);
    if (meta->outputDownscale > 1)
    {
      size >>= scale;
    }
    else
    {
      size <<= scale;
    }
    tasks[taskId].outputX  = size / 4;
    tasks[taskId].outputY  = 1;
    tasks[taskId].outputId = pipeline.declBuffer(size);
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
  auto&                                  main          = terra::get();
  auto&                                  parameterDefs = meta->parameterDef;
  auto&                                  ubo           = pipeline.getUbo();
  auto                                   uboData       = ubo.map(sizeof(EnvParams), meta->uboSize);
  std::vector<GfxDescriptorSet::rhandle> handles((size_t)meta->nbDescriptors);
  for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
  {
    auto& pdef = parameterDefs[i];
    auto& pval = parameters[i];
    switch (pdef.type)
    {
    case ParameterType::eInt2:
    {
      auto val = std::get<int2>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(int2));
    }
    break;
    case ParameterType::eInt:
    {
      auto val = std::get<int>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(int));
    }
    break;
    case ParameterType::eFloat2:
    {
      auto val = std::get<float2>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(float2));
    }
    break;
    case ParameterType::eFloat:
    {
      auto val = std::get<float>(pval);
      std::memcpy(uboData + (size_t)pdef.uboOffset, &val, sizeof(float));
    }
    break;
    case ParameterType::eImage:
    {
      auto& val = std::get<ImageSource>(pval);

      *(float*)(uboData + (size_t)pdef.uboOffset)     = val.defaultValue;
      *(float*)(uboData + (size_t)pdef.uboOffset + 4) = val.uvScale;
      *(int2*)(uboData + (size_t)pdef.uboOffset + 8)  = val.tileConstraintMin;
      *(int2*)(uboData + (size_t)pdef.uboOffset + 16) = val.tileConstraintMax;

      if (std::holds_alternative<ImageDataIdx>(val.source))
        handles[pdef.descriptorIndex].first = main.getImage(std::get<ImageDataIdx>(val.source)).handle;
      else if (std::holds_alternative<hnode>(val.source))
        handles[pdef.descriptorIndex].first = pipeline.getOutputImage(std::get<hnode>(val.source));
      handles[pdef.descriptorIndex].second = val.sampler;
    }
    break;
    case ParameterType::eCurveData:
    {
      auto& val                           = std::get<CurveDataPtr>(pval);
      handles[pdef.descriptorIndex].first = val->handle;
    }
    break;
    case ParameterType::eDataSource:
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