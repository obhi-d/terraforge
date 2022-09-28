
#include "Node.h"
#include "Terra.h"
#include <charconv>
#include <numeric>

namespace terra
{
void ParameterMeta::setTypeFromString(std::string_view stype)
{
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

std::string NodeMeta::writeTextureSampler(std::string_view name)
{

  static inline constexpr char glob[] = R"_(

vec4 sample_{0}(vec4 u, vec4 v, NodeParams np)
{ 
  if (np.has_{0})
  {
    return vec4(
      texture({0}, vec2(u.x, v.x)).r,
      texture({0}, vec2(u.y, v.y)).r,
      texture({0}, vec2(u.z, v.z)).r,
      texture({0}, vec2(u.w, v.w)).r);
  }
  else
  {
    return vec4(np.uniforms.{0});
  }
}


float sample_{0}(float u, float v, NodeParams np)
{ 
  if (np.has_{0})
  {
    return texture({0}, vec2(u.x, v.x)).r;
  }
  else
  {
    return np.uniforms.{0};
  }
}

)_";

  return std::format(glob, name);
}

std::string NodeMeta::writeDataSampler(std::string_view name)
{
  static inline constexpr char glob[] = R"_(

vec4 sample_{0}(int4 x, int4 y, NodeParams np)
{ 
  if (np.has_{0})
  {
    int4 id = pixel_id(x, y, np);
    return vec4(
      {0}.data[uint(id.x)],
      {0}.data[uint(id.y)],
      {0}.data[uint(id.z)],
      {0}.data[uint(id.w)]);
  }
  else
  {
    return vec4(np.uniforms.{0});
  }
}


float sample_{0}(int x, int y, NodeParams np)
{ 
  if (np.has_{0})
  {
    int id = pixel_id(x, y, np);
    return {0}.data[uint(id)];
  }
  else
  {
    return np.uniforms.{0};
  }
}

)_";

  return std::format(glob, name);
}

std::string NodeMeta::writeCurveSampler(std::string_view name) {}

bool NodeMeta::buildShader(Terra& main, RenderDevice& rd)
{
  std::vector<std::string_view> sources;
  auto const&                   shaderBuilder = main.getShaderBuilder();
  std::string                   defines;
  std::string                   generated;
  std::string                   nodeParams;
  // generated content
  uint32_t textureBinding = 0;
  uint32_t storageBinding = 1;
  uint32_t paramOffsets   = sizeof(EnvParams);
  auto     declOrder      = std::vector<uint32_t>(parameterDef.size());

  std::iota(declOrder.begin(), declOrder.end(), 0);
  std::sort(declOrder.begin(), declOrder.end(),
            [this](uint32_t first, uint32_t second)
            {
              return parameterDef[first].type < parameterDef[second].type;
            });

  // print only constants
  for (auto o : declOrder)
  {
    auto& t = parameterDef[o];
    switch (t.type)
    {
    case ParameterType::eInt:
      nodeParams += "int ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case ParameterType::eFloat:
      nodeParams += "float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 4;
      break;
    case ParameterType::eInt2:
      nodeParams += "int2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case ParameterType::eFloat2:
      nodeParams += "float2 ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 8;
      break;
    case ParameterType::eBool:
      nodeParams += "bool ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 1;
      break;
    case ParameterType::eDataSource:
    case ParameterType::eImage:
      nodeParams += "float ";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.availOffset = paramOffsets;
      paramOffsets += 4;
      break;
    }
  }

  // deal with buffers and textures
  for (auto o : declOrder)
  {
    auto& t = parameterDef[o];
    switch (t.type)
    {
    case ParameterType::eImage:
      nodeParams += "bool has_";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.binding = textureBinding++;
      generated += "layout(binding = ";
      generated += std::to_string(t.binding);
      generated += ") sampler2D ";
      generated += t.name;
      generated += ";\n";
      generated += writeTextureSampler(t.name);
      t.availOffset = paramOffsets;
      paramOffsets += 1;
      break;
    case ParameterType::eDataSource:
      nodeParams += "bool has_";
      nodeParams += t.name;
      nodeParams += ";\n";
      t.binding = storageBinding++;
      generated += "layout(binding = ";
      generated += std::to_string(t.binding);
      generated += ") readonly buffer U";
      generated += t.name;
      generated += "{ float4 data[]; }";
      generated += t.name;
      generated += ";\n";
      generated += writeDataSampler(t.name);
      t.availOffset = paramOffsets;
      paramOffsets += 1;
      break;
    case ParameterType::eCurveData:
      t.binding = storageBinding++;
      generated += "layout(binding = ";
      generated += std::to_string(t.binding);
      generated += ") readonly buffer U";
      generated += t.name;
      generated += "{ int npoints; int ncoeff; float data[]; }";
      generated += t.name;
      generated += ";\n";
      generated += writeCurveSampler(t.name);
      break;
    }
  }

  paramSize = paramOffsets;
  sources.emplace_back(shaderBuilder.glslVersion);
  sources.emplace_back(extensions);
  sources.emplace_back(shaderBuilder.typesAndConstants);
  sources.emplace_back(nodeParams);
  sources.emplace_back(generated);
  sources.emplace_back(shaderBuilder.utilityFunctions);
  sources.emplace_back(shaderContent);
  sources.emplace_back(shaderBuilder.fixedResources);

  shader = rd.createComputeShader(sources, GfxCompute::Language::eGLSL);
  if (!shader)
  {
    main.logError("Failed to compile shader.");
    return false;
  }
  return true;
}

} // namespace terra