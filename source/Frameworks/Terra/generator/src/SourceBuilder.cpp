
#include "SourceBuilder.h"
#include "Terra.h"
#include <fmt/format.h>
#include <regex>

namespace terra
{

ShaderProgram::~ShaderProgram()
{
  get().getDevice().destroy(material.program);
  get().getDevice().destroy(material.layout);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram const& other) noexcept
{
  get().getDevice().destroy(material.program);
  get().getDevice().destroy(material.layout);
  material    = other.material;
  outputCount = other.outputCount;
  frame       = other.frame;
  return *this;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
  if (material.program)
    get().getDevice().destroy(material.program);
  if (material.layout)
    get().getDevice().destroy(material.layout);
  material       = other.material;
  outputCount    = other.outputCount;
  frame          = other.frame;
  other.material = {};
  return *this;
}

std::string_view toGlsl(DataTypeEnum type)
{
  switch (type)
  {
  case DataTypeEnum::eInt:
    return "int";
  case DataTypeEnum::eUint:
    return "uint";
  case DataTypeEnum::eFloat:
    return "float";
  case DataTypeEnum::eInt2:
    return "ivec2";
  case DataTypeEnum::eUint2:
    return "uvec2";
  case DataTypeEnum::eFloat2:
    return "vec2";
  case DataTypeEnum::eFloat3:
    return "vec3";
  case DataTypeEnum::eFloat4:
    return "vec4";
  case DataTypeEnum::eMat4:
    return "mat4";
  }
  return "";
}

std::string_view toGlsl(ImageFormatEnum type)
{
  switch (type)
  {
  case ImageFormatEnum::eFloat:
    return "r32f";
  case ImageFormatEnum::eRg32f:
    return "rg32f";
  case ImageFormatEnum::eRgba32f:
    return "rgba32f";
  case ImageFormatEnum::eSrgb8Alpha8:
  case ImageFormatEnum::eRgba8:
    return "rgba8";
  case ImageFormatEnum::eUnorm16:
  case ImageFormatEnum::eSnorm16:
    return "r16";
  case ImageFormatEnum::eUnorm8:
    return "r8";
  }
  return "";
}

std::string_view qualifier(ParamDeclTypeEnum type)
{
  switch (type)
  {
  case ParamDeclTypeEnum::eReadonlyImage:
  case ParamDeclTypeEnum::eReadonlySSBO:
    return "restrict readonly";
  case ParamDeclTypeEnum::eImage:
  case ParamDeclTypeEnum::eSSBO:
    return "restrict";
  case ParamDeclTypeEnum::eWriteonlySSBO:
  case ParamDeclTypeEnum::eWriteonlyImage:
    return "restrict writeonly";
  }
  return "";
}

// ====================== ShaderProgram ====================
void ShaderProgram::touch()
{
  frame = get().frameNumber();
}

// ====================== SourceBuilderAdapter ====================
std::string SourceBuilderAdapter::format(std::string data)
{
  constexpr std::string_view from      = "__id_";
  std::string                to        = std::to_string(id);
  size_t                     start_pos = 0;
  while ((start_pos = data.find(from, start_pos)) != std::string::npos)
  {
    data.replace(start_pos + 1, from.length() - 1, to);
    start_pos += to.length(); // ...
  }
  return data;
}

void SourceBuilderAdapter::pushOptions(ShaderOptions option)
{
  for (uint32_t i = 0; i < option.size(); ++i)
  {
    if (option.isSet(i))
      options += format(fmt::format("#define {}\n", option.name(i)));
  }
}

void SourceBuilderAdapter::pushExtension(std::string_view ext)
{
  extensions += ext;
}

void SourceBuilderAdapter::sampleSSBO(std::string_view name, DataFormat df)
{
  options += fmt::format("#define {}Binding_{} {}\n", name, id, ssboBinding);

  GfxParamLayout::Entry entry;
  entry.index = ssboBinding++;
  entry.type  = GfxBindType::eStorageBuffer;
  entries.push_back(entry);
}

void SourceBuilderAdapter::sampleParam(std::string_view name, DataFormat df)
{
  content += toGlsl(df.scalarSubType);
  if (df.preEval)
  {
    content += format(fmt::format(" l{0}_{1} = sample{0}(input);\n", name, id));
    params.emplace_back(format(fmt::format("l{0}_{1}", name, id)));
  }
  switch (df.declType)
  {
  case ParamDeclTypeEnum::eWriteonlySSBO:
  case ParamDeclTypeEnum::eSSBO:
  case ParamDeclTypeEnum::eReadonlySSBO:
    sampleSSBO(name, df);
    break;
  case ParamDeclTypeEnum::eTexture:
    sampleTexture(name, df);
    break;
  case ParamDeclTypeEnum::eWriteonlyImage:
  case ParamDeclTypeEnum::eImage:
  case ParamDeclTypeEnum::eReadonlyImage:
    sampleImage(name, df);
    break;
  case ParamDeclTypeEnum::eTextureBuffer:
    sampleTextureBuffer(name, df);
    break;
  }
}

void SourceBuilderAdapter::sampleScalar(std::string_view name, DataFormat df)
{
  auto type = toGlsl(df.scalarSubType);
  content += type;
  if (df.preEval)
  {
    content += format(fmt::format(" l{0}_{1} = sample{0}(input);\n", name, id));
    params.emplace_back(format(fmt::format("l{0}_{1}", name, id)));
  }
  ubo += format(fmt::format("  {} bl_{}_{};\n", type, name, id));
  GfxParamLayout::Entry entry;
  entry.index = ssboBinding++;
  switch (df.scalarSubType)
  {
  case DataTypeEnum::eUint:
    entry.type = GfxBindType::eUint;
    break;
  case DataTypeEnum::eUint2:
    entry.type = GfxBindType::eUint2;
    break;
  case DataTypeEnum::eInt2:
    entry.type = GfxBindType::eInt2;
    break;
  case DataTypeEnum::eInt:
    entry.type = GfxBindType::eInt;
    break;
  case DataTypeEnum::eFloat:
    entry.type = GfxBindType::eFloat;
    break;
  case DataTypeEnum::eFloat2:
    entry.type = GfxBindType::eFloat2;
    break;
  case DataTypeEnum::eFloat3:
    entry.type = GfxBindType::eFloat3;
    break;
  case DataTypeEnum::eFloat4:
    entry.type = GfxBindType::eFloat4;
    break;
  case DataTypeEnum::eMat4:
    entry.type = GfxBindType::eMat4;
    break;
  }
  entries.push_back(entry);
}

void SourceBuilderAdapter::computeParam(std::string_view name, DataFormat df)
{
  auto type = toGlsl(df.scalarSubType);
  content += type;
  content += format(fmt::format(" l{0}_{1} = ", name, id));
  params.emplace_back(format(fmt::format("l{0}_{1}", name, id)));
}

void SourceBuilderAdapter::writeOutput(std::string_view name, DataFormat df)
{
  GfxParamLayout::Output output;
  output.format = df.imageFormat;
  switch (df.declType)
  {
  case ParamDeclTypeEnum::eDepthOutput:
    if (df.preEval)
      params.emplace_back("gl_FragDepth");
    output.type = GfxBindType::eDepthBuffer;
    break;
  case ParamDeclTypeEnum::eSSBO:
    output.type = GfxBindType::eStorageBuffer;
    break;
  default:
    resources += format(fmt::format("layout(location={}) {} {};\n", outputIdx++, toGlsl(df.scalarSubType), name));
    if (df.preEval)
      params.emplace_back(format(std::string(name)));
    output.type = GfxBindType::eTexture;
    break;
  }
}

void SourceBuilderAdapter::computeInput(std::string_view call)
{
  content += format(fmt::format("input = {}(input);\n", call));
}

void SourceBuilderAdapter::append(std::string_view fn)
{
  functions += fn;
}

void SourceBuilderAdapter::call(std::string_view node, bool acceptInput)
{
  content += node;
  if (acceptInput)
    content += "(input";
  else
    content += '(';
  bool first = !acceptInput;
  for (auto& p : params)
  {
    if (!first)
      content += ',';
    content += p;
    first = false;
  }
  content += ");\n";
}

void SourceBuilderAdapter::packCommon(std::vector<std::string_view>& snapshots)
{
  if (!extensions.empty())
    snapshots.emplace_back(extensions);
  if (!options.empty())
    snapshots.emplace_back(options);
  if (!resources.empty())
    snapshots.emplace_back(resources);
  if (!ubo.empty())
  {
    ubo = fmt::format("layout(binding = 0) uniform Params\n{{\n{}\n}};\n", ubo);
    snapshots.emplace_back(ubo);
  }
  if (!includes.empty())
    snapshots.emplace_back(resources);
  if (!functions.empty())
    snapshots.emplace_back(functions);
}

GfxProgram::handle SourceBuilderAdapter::makeGpuNode(std::vector<std::string_view>& code)
{
  auto& dev = get().getDevice();
  if (!content.empty())
  {
    content = format(fmt::format(R"_(
void main()
{{
  vec2 input = compute_input(gl_FragCoord.x, gl_FragCoord.y);
  {}
  {}
}})_",
                                 input, content));
  }
  code.emplace_back(content);
  return dev.createFullscreenProgram(code);
}
GfxProgram::handle SourceBuilderAdapter::makePostProcess(std::vector<std::string_view>& code)
{
  auto& dev = get().getDevice();
  code.emplace_back(content);
  return dev.createFullscreenProgram(code);
}

GfxProgram::handle SourceBuilderAdapter::makeShaderProgram(std::vector<std::string_view>& code)
{
  auto& dev = get().getDevice();
  code.emplace_back(content);
  return dev.createProgram(code, GfxProgram::fVertex | GfxProgram::fFragment);
}

ShaderProgram SourceBuilderAdapter::finalize()
{
  std::vector<std::string_view> snapshots;
  packCommon(snapshots);
  GfxProgram::handle program;
  switch (type)
  {
  case SourceType::eFullscreenGraphNode:
    program = makeGpuNode(snapshots);
    break;
  case SourceType::ePostProcess:
    program = makePostProcess(snapshots);
    break;
  case SourceType::eShaderProgram:
    program = makeShaderProgram(snapshots);
    break;
  }

  if (program)
  {
    auto&         dev = get().getDevice();
    ShaderProgram pret;
    pret.material.program = program;
    pret.material.layout  = dev.createLayout(entries, output);
    pret.bindings         = acl::dynamic_array<GfxBindType>((uint32_t)entries.size());
    for (uint32_t i = 0; i < pret.bindings.size(); ++i)
      pret.bindings[i] = entries[i].type;
    pret.outputCount = outputIdx;
    pret.frame       = get().frameNumber();
    return pret;
  }
  return {};
}

// ====================== SourceBuilderBindless ====================
void SourceBuilderBindless::sampleTexture(std::string_view name, DataFormat df)
{
  ubo += format(fmt::format("  uint64_t bl_{}_{};\n", name, id));
  options += fmt::format("#define {0}_{1} sampler2D(bl_{0}_{1})\n", name, id, toGlsl(df.imageFormat));

  GfxParamLayout::Entry entry;
  entry.type = GfxBindType::eTexture;
  entries.push_back(entry);
}

void SourceBuilderBindless::sampleImage(std::string_view name, DataFormat df)
{
  ubo += format(fmt::format("  uint64_t bl_{}_{};\n", name, id));
  options += fmt::format("#define {0}_{1} layout({2}) {3} image2D(bl_{0}_{1})\n", name, id, toGlsl(df.imageFormat),
                         qualifier(df.declType));

  GfxParamLayout::Entry entry;
  entry.type = GfxBindType::eStorageImage;
  entries.push_back(entry);
}

void SourceBuilderBindless::sampleTextureBuffer(std::string_view name, DataFormat df)
{
  ubo += format(fmt::format("  uint64_t bl_{}_{};\n", name, id));
  options += fmt::format("#define {0}_{1} samplerBuffer(bl_{0}_{1})\n", name, id, toGlsl(df.imageFormat));

  GfxParamLayout::Entry entry;
  entry.type = GfxBindType::eTextureBuffer;
  entries.push_back(entry);
}

// ====================== SourceBuilderBindless ====================
void SourceBuilderBindful::sampleTexture(std::string_view name, DataFormat df)
{
  resources += format(fmt::format("layout(binding={2}) {0}_{1};\n", name, id, texBinding));
  GfxParamLayout::Entry entry;
  entry.type  = GfxBindType::eTexture;
  entry.index = texBinding++;
  entries.push_back(entry);
}

void SourceBuilderBindful::sampleImage(std::string_view name, DataFormat df)
{
  resources += format(fmt::format("layout(binding={3}, {2}) {4} {0}_{1};\n", name, id, toGlsl(df.imageFormat),
                                  imageBinding, qualifier(df.declType)));

  GfxParamLayout::Entry entry;
  entry.type  = GfxBindType::eStorageImage;
  entry.index = imageBinding++;
  entries.push_back(entry);
}

void SourceBuilderBindful::sampleTextureBuffer(std::string_view name, DataFormat df)
{
  resources += format(fmt::format("layout(binding={2}) {0}_{1};\n", name, id, texBinding));

  GfxParamLayout::Entry entry;
  entry.type  = GfxBindType::eTextureBuffer;
  entry.index = texBinding++;
  entries.push_back(entry);
}
} // namespace terra