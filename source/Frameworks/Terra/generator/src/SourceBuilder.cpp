
#include "SourceBuilder.h"
#include "Terra.h"
#include <fmt/format.h>
#include <iterator>
#include <regex>

namespace terra
{

ShaderProgram::~ShaderProgram()
{
  if (program)
  {
    get().getDevice().destroy(program);
    program = {};
  }
  if (layout)
  {
    get().getDevice().destroy(layout);
    layout = {};
  }
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
  if (program)
    get().getDevice().destroy(program);
  if (layout)
    get().getDevice().destroy(layout);
  program       = other.program;
  layout        = other.layout;
  frame         = other.frame;
  entries       = std::move(other.entries);
  other.program = {};
  other.layout  = {};
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
  case ParamDeclTypeEnum::eReadonlyImage2D:
  case ParamDeclTypeEnum::eReadonlyStorageBuffer:
    return "restrict readonly";
  case ParamDeclTypeEnum::eImage2D:
  case ParamDeclTypeEnum::eStorageBuffer:
    return "restrict";
  case ParamDeclTypeEnum::eWriteonlyStorageBuffer:
  case ParamDeclTypeEnum::eWriteonlyImage2D:
    return "restrict writeonly";
  }
  return "";
}

GfxBindType toBindType(ParamDeclTypeEnum type)
{
  switch (type)
  {
  case ParamDeclTypeEnum::eDepthOutput:
    return GfxBindType::eDepthBuffer;
  case ParamDeclTypeEnum::eWriteonlyImage2D:
  case ParamDeclTypeEnum::eReadonlyImage2D:
  case ParamDeclTypeEnum::eImage2D:
    return GfxBindType::eStorageImage2D;
  case ParamDeclTypeEnum::eStorageBuffer:
  case ParamDeclTypeEnum::eWriteonlyStorageBuffer:
  case ParamDeclTypeEnum::eReadonlyStorageBuffer:
    return GfxBindType::eStorageBuffer;
  case ParamDeclTypeEnum::eScalar:
    return GfxBindType::eFloat;
  case ParamDeclTypeEnum::eSampler2DShadow:
    return GfxBindType::eShadowTexture2D;
  case ParamDeclTypeEnum::eSampler1D:
    return GfxBindType::eTexture1D;
  case ParamDeclTypeEnum::eSampler2D:
    return GfxBindType::eTexture2D;
  case ParamDeclTypeEnum::eSampler1DArray:
    return GfxBindType::eTexture1DArray;
  case ParamDeclTypeEnum::eTextureBuffer:
    return GfxBindType::eStorageBuffer;
  }
  return GfxBindType::eFloat;
}

// ====================== ShaderProgram ====================
void ShaderProgram::touch()
{
  frame = get().frameNumber();
}

// ====================== SourceBuilderAdapter ====================
SourceBuilderAdapter::SourceBuilderAdapter(SourceType itype) : type(itype) {}
//
// std::string SourceBuilderAdapter::addVariable(std::string_view data)
// {
//   std::string ret = fmt::format("v_{}", data);
//   if (!regex.empty())
//     regex += "|";
//   regex.append(data);
//   return ret;
// }
//
// std::string SourceBuilderAdapter::addContent(std::string_view data)
// {
//   std::string fullrx = "(?![a-zA-Z0-9_])(" + regex + ")(?![a-zA-Z0-9_])";
//   auto        rx     = std::regex(fullrx);
//
//   std::string replace = "v_$1";
//   std::string ss;
//   std::regex_replace(std::back_inserter(ss), data.begin(), data.end(), rx, replace);
//   return ss;
// }

// todo Bindless / bindful both should declare global handles
void SourceBuilderAdapter::options(ShaderOptions option)
{
  for (uint32_t i = 0; i < option.size(); ++i)
  {
    if (option.isSet(i))
    {
      optionHeader += fmt::format("#define {}\n", option.name(i));
    }
  }
}

void SourceBuilderAdapter::option(std::string_view name)
{
  optionHeader += fmt::format("#define {}\n", name);
}

void SourceBuilderAdapter::pushExtension(std::string_view ext)
{
  extensions += ext;
  extensions += '\n';
}

void SourceBuilderAdapter::sampleSSBO(std::string_view name, DataFormat df)
{
  optionHeader += fmt::format("#define {} {}\n", std::string(name) + "_b", ssboBinding);

  GfxParamLayout::Entry entry;
  entry.index = ssboBinding++;
  entry.type  = GfxBindType::eStorageBuffer;
  entries.push_back(entry);
}

void SourceBuilderAdapter::param(std::string_view name, DataFormat df)
{
  if (df.preEval)
  {
    auto lname = localName(name);
    content += fmt::format("{} {} = sample_{}(uv);\n", toGlsl(df.scalarSubType), lname, name);
    params.emplace_back(std::move(lname));
  }

  switch (df.declType)
  {
  case ParamDeclTypeEnum::eWriteonlyStorageBuffer:
  case ParamDeclTypeEnum::eStorageBuffer:
  case ParamDeclTypeEnum::eReadonlyStorageBuffer:
    sampleSSBO(name, df);
    break;
  case ParamDeclTypeEnum::eSampler1D:
  case ParamDeclTypeEnum::eSampler2D:
  case ParamDeclTypeEnum::eSampler1DArray:
  case ParamDeclTypeEnum::eSampler2DShadow:
    sampleTexture(name, df);
    break;
  case ParamDeclTypeEnum::eWriteonlyImage2D:
  case ParamDeclTypeEnum::eImage2D:
  case ParamDeclTypeEnum::eReadonlyImage2D:
    sampleImage(name, df);
    break;
  case ParamDeclTypeEnum::eTextureBuffer:
    sampleTextureBuffer(name, df);
    break;
  default:
    scalar(name, df);
    break;
  }
}

void SourceBuilderAdapter::scalar(std::string_view name, DataFormat df)
{
  std::string loc  = {};
  uint32_t    size = 1;
  if (df.type == DataTypeEnum::eArray)
  {
    loc  = fmt::format("[{}]", (uint32_t)df.maxArraySize);
    size = df.maxArraySize;
  }

  auto type = toGlsl(df.scalarSubType);
  auto sv   = name;
  ubo += fmt::format("  {} {}{};\n", type, sv, loc);
  if (df.preEval)
    params.emplace_back(sv);

  GfxParamLayout::UBOEntry entry;
  entry.name            = name;
  entry.maxArraySize    = df.maxArraySize;
  entry.baseElementSize = ParamHelper::scalarSize(df.scalarSubType);

  uboEntries.push_back(entry);
}

void SourceBuilderAdapter::output(std::string_view name, DataFormat df)
{
  if (type == SourceType::eComputeProgram)
  {
    // outputs are declared as images
    if (df.declType != ParamDeclTypeEnum::eWriteonlyImage2D && df.declType != ParamDeclTypeEnum::eImage2D)
      df.declType = ParamDeclTypeEnum::eWriteonlyImage2D;
    sampleImage(name, df);
  }
  else
  {
    switch (df.declType)
    {
    case ParamDeclTypeEnum::eDepthOutput:
      if (df.preEval)
        params.emplace_back("gl_FragDepth");
      break;
    default:
      resources += fmt::format("#if defined(FragmentShader)\n  layout(location={}) out {} {};\n#endif\n", outputIdx++,
                               toGlsl(df.scalarSubType), name);
      if (df.preEval)
        params.emplace_back(std::string(name));
      break;
    }
  }
}

void SourceBuilderAdapter::computeInput(std::string_view call)
{
  content += fmt::format("uv = {}(uv);\n", call);
}

void SourceBuilderAdapter::append(std::string_view fn)
{
  functions += fn;
}

void SourceBuilderAdapter::call(std::string_view node, bool acceptInput)
{
  content += node;
  if (acceptInput)
    content += "(uv";
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
  if (!optionHeader.empty())
    snapshots.emplace_back(optionHeader);
  if (!ubo.empty())
    snapshots.emplace_back(ubo);
  if (!resources.empty())
    snapshots.emplace_back(resources);
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
    content = fmt::format(R"_(
layout(location = 0) in vec2 fs_UV;
void main()
{{
  vec2 uv = compute_input(fs_UV.x, fs_UV.y);
  {}
  {}
}})_",
                          input, content);
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

GfxProgram::handle SourceBuilderAdapter::makeComputeProgram(std::vector<std::string_view>& code)
{
  auto& dev = get().getDevice();
  code.emplace_back(content);
  return dev.createProgram(code, GfxProgram::fCompute);
}

void SourceBuilderAdapter::packUbo()
{
  if (ubo.empty())
    return;
  ubo = fmt::format("layout(binding = 0) uniform Params {{\n{}\n}};\n", ubo);
}

ShaderProgramPtr SourceBuilderAdapter::finalize()
{
  ShaderProgramPtr pret = std::make_shared<ShaderProgram>();
  packUbo();
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
  case SourceType::eComputeProgram:
    program = makeComputeProgram(snapshots);
    break;
  }

  assert(program);

  auto& dev     = get().getDevice();
  pret->program = program;
  pret->layout  = dev.createLayout(program, entries, uboEntries, pret->refl);
  pret->entries = std::move(entries);
  pret->frame   = get().frameNumber();

  return pret;
}

// ====================== SourceBuilderBindless ====================
void SourceBuilderBindless::sampleTexture(std::string_view name, DataFormat df)
{
  std::string_view sampler = ParamDeclType::toString(df.declType);
  resources += fmt::format("layout(location={}, bindless_sampler) uniform {} {};\n", location, sampler, name);

  GfxParamLayout::Entry entry;
  entry.type  = toBindType(df.declType);
  entry.index = location++;
  entries.push_back(entry);
}

void SourceBuilderBindless::sampleImage(std::string_view name, DataFormat df)
{
  std::string_view type = ParamDeclType::toString(df.declType);
  resources += fmt::format("layout(location={}, bindless_image, {}) uniform {} {} {};\n", location,
                           toGlsl(df.imageFormat), qualifier(df.declType), type, name);

  GfxParamLayout::Entry entry;
  entry.type  = toBindType(df.declType);
  entry.index = location++;
  entries.push_back(entry);
}

void SourceBuilderBindless::sampleTextureBuffer(std::string_view name, DataFormat df)
{
  resources += fmt::format("layout(location={}, bindless_sampler) uniform samplerBuffer {};\n", location, name);

  GfxParamLayout::Entry entry;
  entry.type  = GfxBindType::eTextureBuffer;
  entry.index = location++;
  entries.push_back(entry);
}

// ====================== SourceBuilderBindless ====================
void SourceBuilderBindful::sampleTexture(std::string_view name, DataFormat df)
{
  std::string_view sampler = ParamDeclType::toString(df.declType);
  resources += fmt::format("layout(binding={}) uniform {} {};\n", texBinding, sampler, name);

  GfxParamLayout::Entry entry;
  entry.type  = toBindType(df.declType);
  entry.index = texBinding++;
  entries.push_back(entry);
}

void SourceBuilderBindful::sampleImage(std::string_view name, DataFormat df)
{
  std::string_view type = ParamDeclType::toString(df.declType);
  resources += fmt::format("layout(binding={}, {}) uniform {} {} {};\n", imageBinding, toGlsl(df.imageFormat),
                           qualifier(df.declType), type, name);

  GfxParamLayout::Entry entry;
  entry.type  = toBindType(df.declType);
  entry.index = imageBinding++;
  entries.push_back(entry);
}

void SourceBuilderBindful::sampleTextureBuffer(std::string_view name, DataFormat df)
{
  std::string_view sampler = "samplerBuffer";
  resources += fmt::format("layout(binding={}) uniform {} {};\n", texBinding, sampler, name);

  GfxParamLayout::Entry entry;
  entry.type  = GfxBindType::eTextureBuffer;
  entry.index = texBinding++;
  entries.push_back(entry);
}

} // namespace terra