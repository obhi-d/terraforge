
#include "GfxDevice43.h"
#include "GfxShaderBuilder.h"
#include "GlGfxUtils.h"
#include "Logger.h"
#include "ResourceUtils.h"
#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl43core/gl.h>
#include <glbinding/gl43ext/gl.h>
#include <glm/gtc/type_ptr.hpp>

#ifdef _WIN32
extern "C"
{
  _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

namespace terra
{

namespace glsl
{

constexpr std::string_view fullScreenVS = R"glsl(

layout(location = 0) out highp vec2 fs_UV;
void main() 
{
   vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
   gl_Position = vec4(vertices[gl_VertexID],0,1);
   fs_UV = 0.5 * gl_Position.xy + vec2(0.5);
}

)glsl";

}

namespace gl43 = gl43core;
void MessageCallback(gl43::GLenum source, gl43::GLenum type, gl43::GLuint id, gl43::GLenum severity,
                     gl43::GLsizei length, const gl43::GLchar* message, const void* userParam)
{
  if (type == gl43::GL_DEBUG_TYPE_ERROR)
  {
    logError("GL CALLBACK: {} type = {}, severity = {}, message = {}\n",
             (type == gl43::GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), (int)type, (int)severity,
             (char const*)message);
    throw std::runtime_error("OpenGL error");
  }
}

void GfxDevice43::init()
{
  // auto ext = glbinding::aux::ContextInfo::extensions();
  auto version = glbinding::aux::ContextInfo::version();
  if (version.majorVersion() < 4 || (version.majorVersion() == 4 && version.minorVersion() < 3))
  {
    logError("OpenGL version not supported : {}", version.toString());
    throw std::runtime_error("Failed to initalize device");
  }

  std::set<gl::GLextension> extensions;
  extensions.emplace(gl::GLextension::GL_ARB_bindless_texture);
  if (glbinding::aux::ContextInfo::supported(extensions))
    features.ARB_bindless_texture = GlGfxSupport::eSupported;
  extensions.clear();

  extensions.emplace(gl::GLextension::GL_ARB_clip_control);
  if (glbinding::aux::ContextInfo::supported(extensions))
  {
    features.ARB_clip_control = GlGfxSupport::eSupported;
    gl43ext::glClipControl(gl43::GL_LOWER_LEFT, gl43ext::GL_ZERO_TO_ONE);
  }
  extensions.clear();
  // features.ARB_clip_control = GlGfxSupport::eUnsupported;

  features.version = (int)version.majorVersion() * 100 + (int)version.minorVersion() * 10;
#ifndef NDEBUG
  gl43::glEnable(gl43::GL_DEBUG_OUTPUT);
  gl43::glDebugMessageCallback(MessageCallback, nullptr);
#endif
  logInfo("OpenGL {}.{} - {}", version.majorVersion(), version.minorVersion(), glbinding::aux::ContextInfo::vendor());
}

void GfxDevice43::destroy()
{
  if (fullscreenVS)
    gl43::glDeleteShader(fullscreenVS);
  if (fullscreenVAO)
    gl43::glDeleteVertexArrays(1, &fullscreenVAO);
}

void GfxDevice43::clearBackbuffer(glm::vec4 color, DepthClear depth)
{
  gl43::glClearColor(color.r, color.g, color.b, color.a);
  gl43::ClearBufferMask cbm = gl43::GL_COLOR_BUFFER_BIT;
  if (depth != DepthClear::eNone)
  {
    gl43::glClearDepth(depth == DepthClear::eClearZ_1 ? 1.0f : 0.f);
    cbm |= gl43::GL_DEPTH_BUFFER_BIT;
  }
  gl43::glClear(cbm);
}

void GfxDevice43::flushStates()
{
  state.flush = true;
}
void GfxDevice43::setState(GfxState const& newState)
{
  if (newState.cullMode != state.cullMode || state.flush)
  {
    switch (newState.cullMode)
    {
    case CullMode::eCullBack:
      gl43::glEnable(gl::GLenum::GL_CULL_FACE);
      gl43::glCullFace(gl::GLenum::GL_BACK);
      break;
    case CullMode::eCullFront:
      gl43::glEnable(gl::GLenum::GL_CULL_FACE);
      gl43::glCullFace(gl::GLenum::GL_FRONT);
      break;
    case CullMode::eCullNone:
      gl43::glDisable(gl::GLenum::GL_CULL_FACE);
      break;
    }
    state.cullMode = newState.cullMode;
  }
  {
    for (uint32_t i = 0; i < newState.nbBlendModes; ++i)
    {
      if (state.flush || newState.blend[i].mode != state.blend[i].mode)
      {
        switch (newState.blend[i].mode)
        {
        case BlendMode::eDisabled:
          gl43::glDisablei(gl43::GL_BLEND, i);
          break;
        case BlendMode::eAdditive:
          gl43::glEnablei(gl43::GL_BLEND, i);
          gl43::glBlendEquationi(i, gl43::GL_FUNC_ADD);
          gl43::glBlendFuncSeparatei(i, gl43::GL_SRC_ALPHA, gl43::GL_ONE_MINUS_SRC_ALPHA, gl43::GL_ONE,
                                     gl43::GL_ONE_MINUS_SRC_ALPHA);
          break;
        }
      }
    }
    for (uint32_t i = newState.nbBlendModes; i < state.nbBlendModes; ++i)
      gl43::glDisablei(gl43::GL_BLEND, i);

    state.blend        = newState.blend;
    state.nbBlendModes = newState.nbBlendModes;
  }
  if (newState.depthTest != state.depthTest || state.flush)
  {
    switch (newState.depthTest)
    {
    case DepthTestMode::eDisabled:
      gl43::glDisable(gl43::GL_DEPTH_TEST);
      break;
    case DepthTestMode::eLessEq:
      gl43::glEnable(gl43::GL_DEPTH_TEST);
      gl43::glDepthFunc(gl43::GL_LEQUAL);
      break;
    case DepthTestMode::eGreaterEq:
      gl43::glEnable(gl43::GL_DEPTH_TEST);
      gl43::glDepthFunc(gl43::GL_GEQUAL);
      break;
    }
    state.depthTest = newState.depthTest;
  }
  if (newState.depthWrite != state.depthWrite || state.flush)
  {
    gl43::glDepthMask(newState.depthWrite ? gl43::GL_TRUE : gl43::GL_FALSE);
    state.depthWrite = newState.depthWrite;
  }
  if (newState.scissorsEnabled != state.scissorsEnabled || state.flush)
  {
    if (newState.scissorsEnabled)
      gl43::glEnable(gl43::GL_SCISSOR_TEST);
    else
      gl43::glDisable(gl43::GL_SCISSOR_TEST);
    state.scissorsEnabled = newState.scissorsEnabled;
  }
  if (newState.viewport != state.viewport || state.flush)
  {
    gl43::glViewport(newState.viewport.offset.x, newState.viewport.offset.y, newState.viewport.size.x,
                     newState.viewport.size.y);
    state.viewport = newState.viewport;
  }
  if (state.scissorsEnabled && (newState.scissor != state.scissor || state.flush))
  {
    gl43::glScissor(newState.scissor.offset.x, newState.scissor.offset.y, newState.scissor.size.x,
                    newState.scissor.size.y);
    state.scissor = newState.scissor;
  }
  if (state.polygonOffset != newState.polygonOffset || state.flush)
  {
    if (newState.polygonOffset)
      gl43::glEnable(gl::GL_POLYGON_OFFSET_FILL);
    else
      gl43::glDisable(gl::GL_POLYGON_OFFSET_FILL);
    state.polygonOffset = newState.polygonOffset;
  }
  if (state.polygonOffset && (state.polyOffSlope != newState.polyOffSlope || state.polyOffBias != newState.polyOffBias))
  {
    gl43::glPolygonOffset(newState.polyOffSlope, newState.polyOffBias);
    state.polyOffSlope = newState.polyOffSlope;
    state.polyOffBias  = newState.polyOffBias;
  }
  state.flush = false;
}

GfxBuffer::handle GfxDevice43::createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size)
{
  auto  h     = resources.buffers.emplace();
  auto& res   = resources.buffers.at(h);
  res.usage   = usage;
  res.size    = size;
  res.storage = storage;
  gl43::glGenBuffers(1, &res.glhandle);
  gl43::GLenum storageClass = gl43::GL_STATIC_DRAW;
  switch (storage)
  {
  case GfxStorageClass::eDynamicDeviceAccess:
    storageClass = gl43::GL_DYNAMIC_COPY;
    break;
  case GfxStorageClass::eDeviceAccess:
    storageClass = gl43::GL_STATIC_COPY;
    break;
  case GfxStorageClass::eReadback:
    storageClass = gl43::GL_STREAM_READ;
    break;
  case GfxStorageClass::eDynamicDeviceReadonly:
    storageClass = gl43::GL_DYNAMIC_DRAW;
    break;
  case GfxStorageClass::eStaticDeviceReadonly:
    storageClass = gl43::GL_STATIC_DRAW;
    break;
  }
  auto target = gl43::GL_ARRAY_BUFFER;
  if (usage & GfxBuffer::Usage::fIndex)
    target = gl43::GL_ELEMENT_ARRAY_BUFFER;
  else if (usage & GfxBuffer::Usage::fStorage)
    target = gl43::GL_SHADER_STORAGE_BUFFER;
  else if (usage & GfxBuffer::Usage::fUniform)
    target = gl43::GL_UNIFORM_BUFFER;
  res.target = target;
  gl43::glBindBuffer(target, res.glhandle);
  gl43::glBufferData(target, size, nullptr, storageClass);
  gl43::glBindBuffer(target, 0);
  return h;
}
void GfxDevice43::destroy(GfxBuffer::handle h)
{
  if (!h)
    return;
  auto& res = resources.buffers.at(h);
  gl43::glDeleteBuffers(1, &res.glhandle);
  if (res.gltbo)
    gl43::glDeleteTextures(1, &res.gltbo);
  resources.buffers.erase(h);
}
void GfxDevice43::setTextureParameters(gl::GLenum target, GfxImage::Swizzle swizzle)
{
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_SWIZZLE_R, toGl(swizzle.r));
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_SWIZZLE_G, toGl(swizzle.g));
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_SWIZZLE_B, toGl(swizzle.b));
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_SWIZZLE_A, toGl(swizzle.a));
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_MAG_FILTER, gl43::GL_NEAREST);
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_MIN_FILTER, gl43::GL_NEAREST);
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_WRAP_S, gl43::GL_CLAMP_TO_EDGE);
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_WRAP_T, gl43::GL_CLAMP_TO_EDGE);
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_COMPARE_MODE, gl43::GL_NONE);
  gl43::glTexParameteri(target, gl43::GL_TEXTURE_COMPARE_FUNC, gl43::GL_LEQUAL);
}
GfxImage::handle GfxDevice43::create2DImage(GfxStorageClass storage, uint32_t width, uint32_t height,
                                            ImageFormatEnum format, ubyte_t const* data, GfxImage::Swizzle swizzle,
                                            uint32 mipLevels)
{
  auto  h     = resources.images.emplace();
  auto& res   = resources.images.at(h);
  res.width   = width;
  res.height  = height;
  res.format  = format;
  res.storage = storage;
  gl43::glGenTextures(1, &res.glhandle);
  gl43::glActiveTexture(gl43::GL_TEXTURE0);
  gl43::glBindTexture(gl43::GL_TEXTURE_2D, res.glhandle);
  gl43::glTexStorage2D(gl43::GL_TEXTURE_2D, mipLevels, toGlFormat(format), width, height);
  gl43::glTexSubImage2D(gl43::GL_TEXTURE_2D, 0, 0, 0, width, height, toGlDataFormat(format), toGlType(format), data);
  setTextureParameters(gl43::GL_TEXTURE_2D, swizzle);
  if (mipLevels > 1 && data)
  {
    gl43::glGenerateMipmap(gl43::GL_TEXTURE_2D);
  }
  return h;
}
GfxImage::handle GfxDevice43::create1DImageArray(GfxStorageClass storage, uint32_t width, uint32_t layers,
                                                 ImageFormatEnum format, ubyte_t const* data, GfxImage::Swizzle swizzle,
                                                 uint32 mipLevels)
{
  auto  h     = resources.images.emplace();
  auto& res   = resources.images.at(h);
  res.width   = width;
  res.height  = 1;
  res.layers  = layers;
  res.format  = format;
  res.storage = storage;
  gl43::glGenTextures(1, &res.glhandle);
  gl43::glActiveTexture(gl43::GL_TEXTURE0);
  gl43::glBindTexture(gl43::GL_TEXTURE_1D_ARRAY, res.glhandle);
  gl43::glTexStorage2D(gl43::GL_TEXTURE_1D_ARRAY, mipLevels, toGlFormat(format), width, layers);
  gl43::glTexSubImage2D(gl43::GL_TEXTURE_1D_ARRAY, 0, 0, 0, width, layers, toGlDataFormat(format), toGlType(format),
                        data);
  setTextureParameters(gl43::GL_TEXTURE_1D_ARRAY, swizzle);
  if (mipLevels > 1 && data)
  {
    gl43::glGenerateMipmap(gl43::GL_TEXTURE_1D_ARRAY);
  }
  return h;
}
void GfxDevice43::destroy(GfxImage::handle h)
{
  releaseTexture(h);
}
void GfxDevice43::releaseTexture(GfxImage::handle h)
{
  if (!h)
    return;
  auto& res = resources.images.at(h);
  if ((res.ref--) == 0)
  {
    if (res.fbo)
      gl43::glDeleteFramebuffers(1, &res.fbo);

    if (res.hdev)
      destroy(res.hdev);

    if (res.himg)
      destroy(res.himg);

    for (auto& s : res.hsamplerMap)
    {
      destroy(s.second);
    }

    gl43::glDeleteTextures(1, &res.glhandle);

    res.hsamplerMap.clear();
    res.hdev     = {};
    res.fbo      = 0;
    res.glhandle = 0;

    resources.images.erase(h);
  }
}
GfxSampler::handle GfxDevice43::createSampler(ImageSampling sampling)
{
  auto  h   = resources.samplers.emplace();
  auto& res = resources.samplers.at(h);
  gl43::glGenSamplers(1, &res.glhandle);
  {
    gl43::GLenum minFilter = gl43::GLenum::GL_NEAREST_MIPMAP_NEAREST;
    gl43::GLenum magFilter = gl43::GLenum::GL_NEAREST;
    switch (sampling.sampling)
    {
    case SamplingType::eLinear:
      minFilter = gl43::GLenum::GL_LINEAR_MIPMAP_NEAREST;
      magFilter = gl43::GLenum::GL_LINEAR;
      break;
    case SamplingType::eTrilinear:
      minFilter = gl43::GLenum::GL_LINEAR_MIPMAP_LINEAR;
      magFilter = gl43::GLenum::GL_LINEAR;
      break;
    }

    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_MIN_FILTER, minFilter);
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_MAG_FILTER, magFilter);
  }
  {
    gl43::GLenum tiling = gl43::GLenum::GL_REPEAT;
    switch (sampling.tiling)
    {
    case Tiling::eMirror:
      tiling = gl43::GL_MIRRORED_REPEAT;
      break;
    case Tiling::eClampToEdge:
      tiling = gl43::GL_CLAMP_TO_EDGE;
      break;
    }
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_WRAP_S, tiling);
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_WRAP_T, tiling);
  }
  {
    gl43::GLenum compare = gl43::GL_NONE;
    gl43::GLenum method  = gl43::GL_NEVER;
    // GL_TEXTURE_COMPARE_MODE
    switch (sampling.compare)
    {
    case SampleCompare::eGEq:
      compare = gl43::GL_COMPARE_REF_TO_TEXTURE;
      method  = gl43::GL_GEQUAL;
      break;
    case SampleCompare::eLEq:
      compare = gl43::GL_COMPARE_REF_TO_TEXTURE;
      method  = gl43::GL_LEQUAL;
      break;
    case SampleCompare::eGT:
      compare = gl43::GL_COMPARE_REF_TO_TEXTURE;
      method  = gl43::GL_GREATER;
      break;
    case SampleCompare::eLT:
      compare = gl43::GL_COMPARE_REF_TO_TEXTURE;
      method  = gl43::GL_LESS;
      break;
    }
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_COMPARE_MODE, compare);
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_COMPARE_FUNC, method);
  }
  return h;
}
void GfxDevice43::destroy(GfxSampler::handle h)
{
  if (!h)
    return;

  auto& res = resources.samplers.at(h);
  gl43::glDeleteSamplers(1, &res.glhandle);
  resources.samplers.erase(h);
}
gl43::GLuint GfxDevice43::createShader(ShaderType type, std::span<gl43::GLchar const*> sources,
                                       std::span<gl43::GLint> lengths)
{
  gl43::GLuint glhandle = gl43::glCreateShader(toGlType(type));

#ifndef NDEBUG
  std::string code;
  for (size_t i = 0; i < sources.size(); ++i)
    code += (const char*)sources[i];
  gl43::GLchar const* codes = (gl43::GLchar const*)code.c_str();
  gl43::GLint         size  = (gl43::GLint)code.length();
  gl43::glShaderSource(glhandle, 1, &codes, &size);
#else
  gl43::glShaderSource(glhandle, (gl43::GLsizei)sources.size(), sources.data(), lengths.data());
#endif
  gl43::glCompileShader(glhandle);
  gl43::GLint status = {};
  gl43::glGetShaderiv(glhandle, gl43::GL_COMPILE_STATUS, &status);
  if (status != gl43::GL_TRUE)
  {
    gl43::glGetShaderiv(glhandle, gl43::GL_INFO_LOG_LENGTH, &status);
    std::string   info(status, 0);
    gl43::GLsizei size = {};
    gl43::glGetShaderInfoLog(glhandle, (gl43::GLsizei)info.length(), &size, (gl43::GLchar*)info.data());
    logError("Shader failed to compile: {}", info);
#ifndef NDEBUG
    logError("Shader: \n{}", code);
#endif
    gl43::glDeleteShader(glhandle);
    glhandle = 0;
  }
  return glhandle;
}
GfxDescriptorSetLayout::handle GfxDevice43::createDescriptorSetLayout(
  std::span<GfxDescriptorSetLayout::Descriptor> descriptors)
{
  auto  h             = resources.descriptorSetLayouts.emplace();
  auto& res           = resources.descriptorSetLayouts.at(h);
  res.descriptors     = std::make_unique<GfxDescriptorSetLayout::Descriptor[]>(descriptors.size());
  res.descriptorCount = (uint32_t)descriptors.size();
  std::memcpy(res.descriptors.get(), descriptors.data(), descriptors.size_bytes());
  return h;
}
void GfxDevice43::destroy(GfxDescriptorSetLayout::handle h)
{
  if (!h)
    return;

  resources.descriptorSetLayouts.erase(h);
}
GfxDescriptorSet::handle GfxDevice43::createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout)
{
  auto  h      = resources.descriptorSets.emplace();
  auto& res    = resources.descriptorSets.at(h);
  auto& layout = resources.descriptorSetLayouts.at(descriptorLayout);
  res.layout   = descriptorLayout;
  res.values   = std::make_unique<GfxDescriptorSet::rhandle[]>(layout.descriptorCount);
  std::memset(res.values.get(), 0, layout.descriptorCount * sizeof(GfxDescriptorSet::rhandle));
  return h;
}
void GfxDevice43::destroy(GfxDescriptorSet::handle h)
{
  if (!h)
    return;

  resources.descriptorSets.erase(h);
}
GfxFence::handle GfxDevice43::createFence()
{
  auto  h   = resources.fences.emplace();
  auto& res = resources.fences.at(h);
  res.sync  = gl43::glFenceSync(gl43::GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  return GfxFence::handle(h);
}
void GfxDevice43::syncFence(GfxFence::handle h)
{
  auto& res = resources.fences.at(h);
  gl43::glWaitSync(res.sync, 0, gl43::GL_TIMEOUT_IGNORED);
  resources.fences.erase(h);
}
GfxProgram::handle GfxDevice43::createProgram(std::span<ShaderOptions> options, ShaderBuilder const& sources)
{
  std::string optionStr = fmt::format("#version {}\n", this->features.version);
  for (auto& option : options)
  {
    for (uint32_t i = 0; i < option.size(); ++i)
    {
      fmt::format_to(std::back_inserter(optionStr), "#define {} {}\n", option.name(i), option.value(i));
    }
  }

  auto&                            sb = (GfxShaderBuilder&)sources;
  std::vector<gl43::GLchar const*> sourceFiles;
  std::vector<gl43::GLint>         sourceLengths;

  sourceFiles.emplace_back((gl43::GLchar const*)optionStr.c_str());
  sourceLengths.emplace_back((gl43::GLint)optionStr.length());

  sourceFiles.emplace_back((gl43::GLchar const*)sb.declarations.c_str());
  sourceLengths.emplace_back((gl43::GLint)sb.declarations.length());

  GfxProgram::handle h   = resources.programs.emplace();
  auto&              res = resources.programs.at(h);
  for (uint32_t i = 0; i < ShaderTypeCount; ++i)
  {
    if (!sb.output[i].empty())
    {
      sourceFiles.emplace_back((gl43::GLchar const*)sb.output[i].c_str());
      sourceLengths.emplace_back((gl43::GLint)sb.output[i].length());
      res.shaders[i] = createShader((ShaderType)i, sourceFiles, sourceLengths);
      if (!res.shaders[i])
      {
        {
          std::ofstream cc(getMediaPath() / "error_sh.glsl");
          for (auto const& sf : sourceFiles)
            cc << (char const*)sf;
        }
        for (uint32_t i = 0; i < ShaderTypeCount; ++i)
        {
          if (res.shaders[i])
            gl43::glDeleteShader(res.shaders[i]);
        }
        destroy(GfxProgram::handle(h));
        return {};
      }
      sourceFiles.pop_back();
      sourceLengths.pop_back();
    }
  }
  res.glhandle = gl43::glCreateProgram();
  for (uint32_t i = 0; i < ShaderTypeCount; ++i)
  {
    if (res.shaders[i])
      gl43::glAttachShader(res.glhandle, res.shaders[i]);
  }
  gl43::glLinkProgram(res.glhandle);
  gl43::GLint status = {};
  gl43::glGetProgramiv(res.glhandle, gl43::GL_LINK_STATUS, &status);
  if (status != gl43::GL_TRUE)
  {
    gl43::glGetProgramiv(res.glhandle, gl43::GL_INFO_LOG_LENGTH, &status);
    std::string   info(status, 0);
    gl43::GLsizei size = {};
    gl43::glGetProgramInfoLog(res.glhandle, (gl43::GLsizei)info.length(), &size, (gl43::GLchar*)info.data());
    logError("Program failed to link: {}", info);
    for (uint32_t i = 0; i < ShaderTypeCount; ++i)
    {
      if (res.shaders[i])
        gl43::glDeleteShader(res.shaders[i]);
    }
    destroy(GfxProgram::handle(h));
    h = {};
    return h;
  }
  gl43::glValidateProgram(res.glhandle);
  gl43::glGetProgramiv(res.glhandle, gl43::GL_VALIDATE_STATUS, &status);
  if (status != gl43::GL_TRUE)
  {
    logError("Program validation failed: {}", (uint32_t)h);
    destroy(GfxProgram::handle(h));
    h = {};
  }
  return h;
}
void GfxDevice43::destroy(GfxProgram::handle h)
{
  if (!h)
    return;

  auto& res = resources.programs.at(h);
  for (uint32_t i = 0; i < ShaderTypeCount; ++i)
  {
    if (res.shaders[i])
      gl43::glDeleteShader(res.shaders[i]);
  }
  gl43::glDeleteProgram(res.glhandle);
  resources.programs.erase(h);
}
GfxMesh::handle GfxDevice43::createMeshLayout(GfxMesh::Layout const& mesh)
{
  auto exists = resources.meshMap.find(mesh);
  if (exists != resources.meshMap.end())
  {
    auto& res = resources.meshes.at(exists->second);
    res.usageCounter++;
    return exists->second;
  }
  auto  h          = resources.meshes.emplace();
  auto& res        = resources.meshes.at(h);
  auto  it         = resources.meshMap.emplace(mesh, h);
  res.desc         = &it.first->first;
  res.usageCounter = 1;
  gl43::glGenVertexArrays(1, &res.glhandle);
  gl43::glBindVertexArray(res.glhandle);
  for (uint32_t i = 0; i < mesh.vertexBufferCount; ++i)
  {
    for (uint32_t s = 0; s < mesh.vertexBuffers[i].elementCount; ++s)
    {
      auto& el = mesh.vertexBuffers[i].elements[s];
      if (el.shaderBinding >= 0)
      {
        auto attribCount = el.shaderBinding;
        switch (el.format)
        {
        case GfxVertexFormat::eFloat:
          gl43::glVertexAttribFormat(attribCount, 1, gl43::GL_FLOAT, gl43::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eFloat2:
          gl43::glVertexAttribFormat(attribCount, 2, gl43::GL_FLOAT, gl43::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eFloat3:
          gl43::glVertexAttribFormat(attribCount, 3, gl43::GL_FLOAT, gl43::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eUint32:
          gl43::glVertexAttribFormat(attribCount, 1, gl43::GL_UNSIGNED_INT, gl43::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eUnormUint32:
          gl43::glVertexAttribFormat(attribCount, 1, gl43::GL_UNSIGNED_INT, gl43::GL_TRUE, el.relOffset);
          break;
        case GfxVertexFormat::eUnormByte4:
          gl43::glVertexAttribFormat(attribCount, 4, gl43::GL_UNSIGNED_BYTE, gl43::GL_TRUE, el.relOffset);
          break;
        }
        gl43::glVertexAttribBinding(attribCount, i);
        gl43::glEnableVertexAttribArray(attribCount);
      }
      else
        gl43::glDisableVertexAttribArray(-el.shaderBinding);
    }
  }
  return GfxMesh::handle(h);
}
void GfxDevice43::destroy(GfxMesh::handle h)
{
  if (!h)
    return;

  auto& res = resources.meshes.at(h);
  if (!--res.usageCounter)
  {
    resources.meshMap.erase(*res.desc);
    gl43::glDeleteVertexArrays(1, &res.glhandle);
    resources.meshes.erase(h);
  }
}
ubyte_t* GfxDevice43::mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size)
{
  auto& res = resources.buffers.at(buffer);
  assert(offset + size <= res.size);
  gl43::glBindBuffer(res.target, res.glhandle);
  return (ubyte_t*)gl43::glMapBufferRange(res.target, offset, size,
                                          gl43::MapBufferAccessMask::GL_MAP_WRITE_BIT |
                                            gl43::MapBufferAccessMask::GL_MAP_INVALIDATE_RANGE_BIT);
}
void GfxDevice43::unmapBuffer(GfxBuffer::handle buffer)
{
  auto& res = resources.buffers.at(buffer);
  gl43::glUnmapBuffer(res.target);
}
void GfxDevice43::updateImage(GfxImage::handle image, std::span<ubyte_t const> data)
{
  auto& res = resources.images.at(image);
  gl43::glActiveTexture(gl43::GL_TEXTURE0);
  gl43::glBindTexture(gl43::GL_TEXTURE_2D, res.glhandle);
  gl43::glTexSubImage2D(gl43::GL_TEXTURE_2D, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format),
                        toGlType(res.format), data.data());
}
void GfxDevice43::updateDescriptorSet(GfxDescriptorSet::handle h, std::span<GfxDescriptorSet::rhandle> handles)
{
  auto& res = resources.descriptorSets.at(h);
  std::memcpy(res.values.get(), handles.data(), handles.size_bytes());
}
void GfxDevice43::readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out)
{
  auto& res = resources.buffers.at(buffer);
  gl43::glBindBuffer(res.target, res.glhandle);
  gl43::glGetBufferSubData(res.target, offset, out.size_bytes(), out.data());
}
void GfxDevice43::readImage(GfxImage::handle buffer, std::span<ubyte_t> out)
{
  auto& res = resources.images.at(buffer);
  gl43::glActiveTexture(gl43::GL_TEXTURE0);
  gl43::glBindTexture(gl43::GL_TEXTURE_2D, res.glhandle);
  gl43::glTexSubImage2D(gl43::GL_TEXTURE_2D, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format),
                        toGlType(res.format), out.data());
}
void GfxDevice43::dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                                  uint32_t numGroupY)
{
  gl43::glUseProgram(resources.programs[shader].glhandle);
  bindResources(descriptorSet);
  gl43::glDispatchCompute(numGroupX, numGroupY, 1);
}

void GfxDevice43::dispatchCompute(GfxMaterial2 const& material, uint32_t numGroupX, uint32_t numGroupY)
{
  gl43::glUseProgram(resources.programs[material.program].glhandle);
  apply(material.layout, material.ubo, material.bindings);
  gl43::glDispatchCompute(numGroupX, numGroupY, 1);
}
void GfxDevice43::barrier(GfxBarrierFlags flags)
{
  gl43::MemoryBarrierMask mask = gl43::MemoryBarrierMask::GL_NONE_BIT;
  if (flags & GfxBarrierFlags::fAtomicCounter)
    mask |= gl43::MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fImageAccess)
    mask |= gl43::MemoryBarrierMask::GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fStorageBuffer)
    mask |= gl43::MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fTextureAccess)
    mask |= gl43::MemoryBarrierMask::GL_TEXTURE_FETCH_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fUniformBuffer)
    mask |= gl43::MemoryBarrierMask::GL_UNIFORM_BARRIER_BIT;
  gl43::glMemoryBarrier(mask);
}
std::shared_ptr<ShaderBuilder> GfxDevice43::createShaderBuilder(ShaderLang)
{
  return std::make_shared<GfxShaderBuilder>(features);
}

std::shared_ptr<SourceBuilder> GfxDevice43::createSourceBuilder(ShaderLang, SourceType type)
{
  if (features.ARB_bindless_texture == GlGfxSupport::eSupported)
    return std::make_shared<SourceBuilderBindless>(type);
  else
    return std::make_shared<SourceBuilderBindful>(type);
}

void GfxDevice43::draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& mat)
{
  gl43::glUseProgram(resources.programs[mat.program].glhandle);
  apply(mat);
  draw(drawDesc);
}

void GfxDevice43::draw(GfxMesh::Draw const& drawDesc, GfxMaterial2 const& material)
{
  gl43::glUseProgram(resources.programs[material.program].glhandle);
  apply(material.layout, material.ubo, material.bindings);
  draw(drawDesc);
}

void GfxDevice43::apply(GfxMaterial const& mat)
{
  bindResources(mat.descriptorSet);
}

void GfxDevice43::draw(GfxMesh::Draw const& drawDesc)
{
  auto& mesh = resources.meshes[drawDesc.layout];
  gl43::glBindVertexArray(mesh.glhandle);
  for (uint32_t i = 0; i < mesh.desc->vertexBufferCount; ++i)
  {
    auto bufferOffset = drawDesc.vertexBuffers[i].offset;
    // if (mesh.vertexBuffers[i] != drawDesc.vertexBuffers[i].handle || bufferOffset != mesh.vertexBufferOffsets[i])
    {
      auto oglBuffer = resources.buffers[drawDesc.vertexBuffers[i].handle].glhandle;
      gl43::glBindVertexBuffer(i, oglBuffer, bufferOffset, mesh.desc->vertexBuffers[i].stride);
      mesh.vertexBuffers[i]       = drawDesc.vertexBuffers[i].handle;
      mesh.vertexBufferOffsets[i] = bufferOffset;
    }
  }

  auto mode = toGLDrawMode(drawDesc.type);
  if (drawDesc.indexCount > 0)
  {
    // if (mesh.elementBuffer != drawDesc.indexBuffer.handle)
    {
      auto indexBuffer = resources.buffers[drawDesc.indexBuffer.handle].glhandle;
      gl43::glBindBuffer(gl43::GLenum::GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
      mesh.elementBuffer = drawDesc.indexBuffer.handle;
    }
    if (drawDesc.baseVertex == 0)
      gl43::glDrawElements(mode, drawDesc.indexCount,
                           drawDesc.indexBufferStride == 2 ? gl43::GL_UNSIGNED_SHORT : gl43::GL_UNSIGNED_INT,
                           (void const*)(uintptr_t)drawDesc.indexBuffer.offset);
    else
      gl43::glDrawElementsBaseVertex(mode, drawDesc.indexCount,
                                     drawDesc.indexBufferStride == 2 ? gl43::GL_UNSIGNED_SHORT : gl43::GL_UNSIGNED_INT,
                                     (void const*)(uintptr_t)drawDesc.indexBuffer.offset, drawDesc.baseVertex);
  }
  else
    gl43::glDrawArrays(mode, 0, drawDesc.vertexCount);
  gl43::glBindVertexArray(0);
}

void GfxDevice43::bindResources(GfxDescriptorSet::handle descriptorSet)
{
  auto& res    = resources.descriptorSets[descriptorSet];
  auto& layout = resources.descriptorSetLayouts[res.layout];
  for (uint32_t i = 0; i < layout.descriptorCount; ++i)
  {
    switch (layout.descriptors[i].type)
    {
    case GfxDescriptorType::eBuffer:
      gl43::glBindBufferBase(gl43::GL_SHADER_STORAGE_BUFFER, layout.descriptors[i].binding,
                             resources.buffers[res.values[i].first].glhandle);
      break;
    case GfxDescriptorType::eConstants:
      gl43::glBindBufferBase(gl43::GL_UNIFORM_BUFFER, layout.descriptors[i].binding,
                             resources.buffers[res.values[i].first].glhandle);
      break;
    case GfxDescriptorType::eImage:
      gl43::glBindImageTexture(layout.descriptors[i].binding, resources.images[res.values[i].first].glhandle, 0,
                               gl43::GL_FALSE, 0, toGlMapBit(layout.descriptors[i].access), gl43::GL_RG32F);
      break;
    case GfxDescriptorType::eTexture:
      if (res.values[i].second)
        gl43::glBindSampler(layout.descriptors[i].binding, resources.samplers[res.values[i].second].glhandle);
      gl43::glActiveTexture(gl43::GL_TEXTURE0 + layout.descriptors[i].binding);
      if (res.values[i].first)
        gl43::glBindTexture(gl43::GL_TEXTURE_2D, resources.images[res.values[i].first].glhandle);
      break;
    }
  }
}
void GfxDevice43::applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle layout) {}

GfxParamLayout::handle GfxDevice43::createLayout(GfxProgram::handle                        program,
                                                 std::span<GfxParamLayout::Entry const>    entries,
                                                 std::span<GfxParamLayout::UBOEntry const> uboEntries,
                                                 GfxParamLayout::UBOReflect&               uboRefl)
{
  auto  h         = resources.bindlessLayout.emplace();
  auto& res       = resources.bindlessLayout.at(h);
  auto  nbEntries = (uint32_t)entries.size();
  res.entries     = acl::dynamic_array<GfxBindlessLayoutGl::Entry>(nbEntries);
  std::memcpy(res.entries.data(), entries.data(), entries.size_bytes());

  auto prog = resources.programs[program].glhandle;

  {
    gl::GLuint loc = gl43::glGetProgramResourceIndex(prog, gl::GL_UNIFORM_BLOCK, "Params");
    if (loc == gl::GL_INVALID_INDEX)
      uboRefl.uboSize = 0;
    else
    {
      gl::GLenum props  = {gl::GL_BUFFER_DATA_SIZE};
      gl::GLint  values = {};
      // get total buffer size
      gl43::glGetProgramResourceiv(prog, gl::GL_UNIFORM_BLOCK, loc, 1, &props, 1, nullptr, &values);
      uboRefl.uboSize = (uint32_t)values;
    }
  }

  for (auto& e : uboEntries)
  {
    gl::GLuint loc = gl43::glGetProgramResourceIndex(prog, gl::GL_UNIFORM, e.name.c_str());
    if (loc == gl::GL_INVALID_INDEX)
    {
      GfxParamLayout::UBOOffset offset;
      offset.baseElementSize = 0;
      offset.arrayStride     = 0;
      offset.offset          = 0xffffffff;
      uboRefl.offsets.emplace_back(offset);
    }
    else
    {
      gl::GLenum props[]   = {gl::GL_ARRAY_STRIDE, gl::GL_OFFSET};
      gl::GLint  values[2] = {};
      gl43::glGetProgramResourceiv(prog, gl::GL_UNIFORM, loc, 2, props, 2, nullptr, values);
      GfxParamLayout::UBOOffset offset;
      offset.baseElementSize = e.baseElementSize;
      offset.arrayStride     = (uint16_t)values[0];
      offset.offset          = (uint16_t)values[1];
      uboRefl.offsets.emplace_back(offset);
    }
  }

  return h;
}

void GfxDevice43::destroy(GfxParamLayout::handle h)
{
  if (!h)
    return;
  resources.bindlessLayout.erase(h);
}

BindlessHandleGl::handle GfxDevice43::makeBindless(GfxImageGl const& texture, GfxSamplerGl const& sampler)
{
  BindlessHandleGl bhdl;
  bhdl.hdev = gl43ext::glGetTextureSamplerHandleARB(texture.glhandle, sampler.glhandle);
  bhdl.type = BindlessHandleType::eTexture;
  return resources.bindlessHandles.emplace(bhdl);
}

BindlessHandleGl::handle GfxDevice43::makeBindless(GfxImageGl const& texture)
{
  BindlessHandleGl bhdl;
  bhdl.hdev = gl43ext::glGetTextureHandleARB(texture.glhandle);
  bhdl.type = BindlessHandleType::eTexture;
  return resources.bindlessHandles.emplace(bhdl);
}

BindlessHandleGl::handle GfxDevice43::makeBindless(GfxBufferGl const& texture)
{
  BindlessHandleGl bhdl;
  bhdl.hdev = gl43ext::glGetTextureHandleARB(texture.gltbo);
  bhdl.type = BindlessHandleType::eTexture;
  return resources.bindlessHandles.emplace(bhdl);
}

BindlessHandleGl::handle GfxDevice43::makeBindless(GfxImageGl const& res, StorageImage const& image)
{
  BindlessHandleGl bhdl;
  bhdl.hdev = gl43ext::glGetImageHandleARB(res.glhandle, 0, image.layered, image.layer, toGlFormat(res.format));
  bhdl.type = BindlessHandleType::eImage;
  return resources.bindlessHandles.emplace(bhdl);
}

void GfxDevice43::makeResident(BindlessHandleGl::handle hdl, GfxAccess access)
{
  auto& bhdl = resources.bindlessHandles[hdl];
  assert(bhdl.type == BindlessHandleType::eImage);
  if (!bhdl.resident)
    gl43ext::glMakeImageHandleResidentARB(bhdl.hdev, toGl(GfxAccess::eReadWrite));

  bhdl.residentFrame = frame;
  bhdl.resident      = true;
  // bhdl.access        = access;

  if (!bhdl.active)
  {
    resources.activeResidents.emplace_back(hdl);
    bhdl.active = true;
  }
}

void GfxDevice43::makeResident(BindlessHandleGl::handle hdl)
{
  auto& bhdl = resources.bindlessHandles[hdl];
  if (!bhdl.resident)
    gl43ext::glMakeTextureHandleResidentARB(bhdl.hdev);
  bhdl.residentFrame = frame;
  bhdl.resident      = true;
  if (!bhdl.active)
  {
    resources.activeResidents.emplace_back(hdl);
    bhdl.active = true;
  }
}

void GfxDevice43::destroy(BindlessHandleGl::handle hdl)
{
  if (!hdl)
    return;
  auto& bhdl = resources.bindlessHandles[hdl];
  if (bhdl.resident)
  {
    if (bhdl.type == BindlessHandleType::eTexture)
      gl43ext::glMakeTextureHandleNonResidentARB(bhdl.hdev);
    else
      gl43ext::glMakeImageHandleNonResidentARB(bhdl.hdev);
  }
  if (bhdl.active)
  {
    for (auto it = resources.activeResidents.begin(); it != resources.activeResidents.end(); ++it)
    {
      if (*it == hdl)
      {
        resources.activeResidents.erase(it);
        break;
      }
    }
  }
  resources.bindlessHandles.erase(hdl);
}

void GfxDevice43::bindSampledTexture(gl::GLenum target, uint32_t index, SampledTexture const& tex)
{
  if (features.ARB_bindless_texture == GlGfxSupport::eSupported)
  {
    BindlessHandleGl::handle hdev;
    auto&                    texture = resources.images[tex.texture];
    if (tex.sampler)
    {
      auto s = texture.hsamplerMap.find(tex.sampler);
      if (s == texture.hsamplerMap.end())
      {
        auto& sampler                    = resources.samplers[tex.sampler];
        hdev                             = makeBindless(texture, sampler);
        texture.hsamplerMap[tex.sampler] = hdev;
      }
      else
        hdev = s->second;
    }
    else
    {
      hdev = texture.hdev;
      if (!hdev)
        hdev = texture.hdev = makeBindless(texture);
    }

    auto& bhdl = resources.bindlessHandles[hdev];
    makeResident(hdev);
    gl43ext::glUniformHandleui64ARB(index, resources.bindlessHandles[hdev].hdev);
  }
  else
  {
    auto& texture = resources.images[tex.texture];

    gl43::glActiveTexture(gl::GL_TEXTURE0 + index);
    gl43::glBindTexture(target, texture.glhandle);
    if (tex.sampler)
    {
      auto& sampler = resources.samplers[tex.sampler];
      gl43::glBindSampler(index, sampler.glhandle);
    }
  }
}

void GfxDevice43::bindTextureBuffer(uint32_t index, TextureBuffer const& buffer)
{
  auto& res = resources.buffers[buffer.buffer];
  if (!res.gltbo)
  {
    gl43::glGenTextures(1, &res.gltbo);
    gl43::glBindTexture(gl43::GL_TEXTURE_BUFFER, res.gltbo);
    gl43::glTexBuffer(gl43::GL_TEXTURE_BUFFER, toGlFormat(buffer.format), res.glhandle);
  }
  if (features.ARB_bindless_texture == GlGfxSupport::eSupported)
  {
    if (!res.hdev)
      res.hdev = makeBindless(res);
    makeResident(res.hdev);
    gl43ext::glUniformHandleui64ARB(index, resources.bindlessHandles[res.hdev].hdev);
  }
  else
  {
    gl43::glActiveTexture(gl::GL_TEXTURE0 + index);
    gl43::glBindTexture(gl43::GL_TEXTURE_BUFFER, res.gltbo);
  }
}

void GfxDevice43::apply(GfxParamLayout::handle descriptorLayout, UboData const& uboData, Blob const& bindings)
{
  auto  reader  = bindings.reader();
  auto& layout  = resources.bindlessLayout[descriptorLayout];
  auto& entries = layout.entries;
  if (uboData.size() > 0)
  {
    gl::GLuint ubo;
    gl43::glGenBuffers(1, &ubo);
    gl43::glBindBuffer(gl43::GL_UNIFORM_BUFFER, ubo);
    gl43::glBufferData(gl43::GL_UNIFORM_BUFFER, uboData.size(), uboData.data(), gl43::GL_STATIC_DRAW);
    gl43::glBindBuffer(gl43::GL_UNIFORM_BUFFER, 0);
    gl43::glBindBufferRange(gl::GL_UNIFORM_BUFFER, 0, ubo, 0, uboData.size());
    uboList.push_back(ubo);
  }

  for (auto& e : entries)
  {
    switch (e.type)
    {
    case GfxBindType::eStorageBuffer:
    {
      auto const& ssbo = reader.read<StorageBuffer>();
      gl43::glBindBufferRange(gl::GL_SHADER_STORAGE_BUFFER, e.index, resources.buffers[ssbo.buffer].glhandle,
                              ssbo.offset, ssbo.size);
      break;
    }
    case GfxBindType::eShadowTexture2D:
    case GfxBindType::eTexture2D:
      bindSampledTexture(gl::GL_TEXTURE_2D, e.index, reader.read<SampledTexture>());
      break;
    case GfxBindType::eTexture1D:
      bindSampledTexture(gl::GL_TEXTURE_1D, e.index, reader.read<SampledTexture>());
      break;
    case GfxBindType::eTexture1DArray:
      bindSampledTexture(gl::GL_TEXTURE_1D_ARRAY, e.index, reader.read<SampledTexture>());
      break;

    case GfxBindType::eTextureBuffer:
      bindTextureBuffer(e.index, reader.read<TextureBuffer>());
      break;
    case GfxBindType::eStorageImage2D:
    {
      auto const& image = reader.read<StorageImage>();
      auto&       res   = resources.images[image.texture];
      if (features.ARB_bindless_texture == GlGfxSupport::eSupported)
      {
        if (!res.himg)
          res.himg = makeBindless(res, image);
        assert(image.layered == false); // TODO Should add support for multiple image handle per texture, or have a
                                        // image type which stores the data
        makeResident(res.himg, image.access);
        gl43ext::glUniformHandleui64ARB(e.index, resources.bindlessHandles[res.himg].hdev);
      }
      else
      {
        gl43::glBindImageTexture(e.index, res.glhandle, 0, image.layered, image.layer, toGl(image.access),
                                 toGlFormat(res.format));
      }
    }
    break;
    case GfxBindType::eInt:
    case GfxBindType::eUint:
    case GfxBindType::eInt2:
    case GfxBindType::eUint2:
    case GfxBindType::eFloat:
    case GfxBindType::eFloat2:
    case GfxBindType::eFloat3:
    case GfxBindType::eFloat4:
    case GfxBindType::eMat4:
    case GfxBindType::eFloatArray:
    case GfxBindType::eIntArray:
    case GfxBindType::eUintArray:
      break;
    }
  }
}

GfxProgram::handle GfxDevice43::createProgram(std::span<std::string_view> code, uint32_t activeStages)
{
  GfxProgram::handle h   = resources.programs.emplace();
  auto&              res = resources.programs.at(h);

  constexpr std::string_view stages[GfxProgram::MaxStage] = {"#define VertexShader 1\n", "#define GeometryShader 1\n",
                                                             "#define FragmentShader 1\n", "#define ComputeShader 1\n"};

  std::string version = fmt::format("#version {}\n", this->features.version);

  res.glhandle = gl43::glCreateProgram();
  std::vector<gl43::GLchar const*> sourceFiles;
  std::vector<gl43::GLint>         sourceLengths;
  for (uint32_t i = 0; i < GfxProgram::MaxStage; ++i)
  {
    if (!(activeStages & (1 << i)))
      continue;
    sourceFiles.emplace_back((gl43::GLchar const*)version.c_str());
    sourceLengths.emplace_back((gl43::GLint)version.size());
    sourceFiles.emplace_back((gl43::GLchar const*)stages[i].data());
    sourceLengths.emplace_back((gl43::GLint)stages[i].size());
    for (auto c : code)
    {
      sourceFiles.emplace_back((gl43::GLchar const*)c.data());
      sourceLengths.emplace_back((gl43::GLint)c.size());
    }
    res.shaders[i] = createShader((ShaderType)i, sourceFiles, sourceLengths);
    if (res.shaders[i])
    {
      gl43::glAttachShader(res.glhandle, res.shaders[i]);
    }
    sourceFiles.clear();
    sourceLengths.clear();
  }
  gl43::glLinkProgram(res.glhandle);
  gl43::GLint status = {};
  gl43::glGetProgramiv(res.glhandle, gl43::GL_LINK_STATUS, &status);
  if (status != gl43::GL_TRUE)
  {
    gl43::glGetProgramiv(res.glhandle, gl43::GL_INFO_LOG_LENGTH, &status);
    std::string   info(status, 0);
    gl43::GLsizei size = {};
    gl43::glGetProgramInfoLog(res.glhandle, (gl43::GLsizei)info.length(), &size, (gl43::GLchar*)info.data());
    logError("Program failed to link: {}", info);
    for (uint32_t i = 0; i < ShaderTypeCount; ++i)
    {
      if (res.shaders[i])
        gl43::glDeleteShader(res.shaders[i]);
    }
    destroy(GfxProgram::handle(h));
    h = {};
    return h;
  }
  gl43::glValidateProgram(res.glhandle);
  gl43::glGetProgramiv(res.glhandle, gl43::GL_VALIDATE_STATUS, &status);
  if (status != gl43::GL_TRUE)
  {
    logError("Program validation failed: {}", (uint32_t)h);
    destroy(GfxProgram::handle(h));
    h = {};
  }
  return h;
}

GfxProgram::handle GfxDevice43::createFullscreenProgram(std::span<std::string_view> code)
{
  GfxProgram::handle h   = resources.programs.emplace();
  auto&              res = resources.programs.at(h);

  std::vector<gl43::GLchar const*> sourceFiles;
  std::vector<gl43::GLint>         sourceLengths;

  std::string                version  = fmt::format("#version {}\n", this->features.version);
  constexpr std::string_view fragment = "#define FragmentShader\n";

  if (!fullscreenVS)
  {
    sourceFiles.emplace_back((gl43::GLchar const*)version.c_str());
    sourceLengths.emplace_back((gl43::GLint)version.size());
    sourceFiles.emplace_back((gl43::GLchar const*)glsl::fullScreenVS.data());
    sourceLengths.emplace_back((gl43::GLint)glsl::fullScreenVS.size());
    fullscreenVS = createShader(ShaderType::eVertex, sourceFiles, sourceLengths);
    sourceFiles.clear();
    sourceLengths.clear();
  }

  sourceFiles.emplace_back((gl43::GLchar const*)version.c_str());
  sourceLengths.emplace_back((gl43::GLint)version.size());
  sourceFiles.emplace_back((gl43::GLchar const*)fragment.data());
  sourceLengths.emplace_back((gl43::GLint)fragment.size());
  for (auto c : code)
  {
    sourceFiles.emplace_back((gl43::GLchar const*)c.data());
    sourceLengths.emplace_back((gl43::GLint)c.size());
  }

  res.shaders[0] = createShader(ShaderType::eFragment, sourceFiles, sourceLengths);
  if (res.shaders[0])
  {
    res.glhandle = gl43::glCreateProgram();
    gl43::glAttachShader(res.glhandle, fullscreenVS);
    gl43::glAttachShader(res.glhandle, res.shaders[0]);

    gl43::glLinkProgram(res.glhandle);
    gl43::GLint status = {};
    gl43::glGetProgramiv(res.glhandle, gl43::GL_LINK_STATUS, &status);
    if (status != gl43::GL_TRUE)
    {
      gl43::glGetProgramiv(res.glhandle, gl43::GL_INFO_LOG_LENGTH, &status);
      std::string   info(status, 0);
      gl43::GLsizei size = {};
      gl43::glGetProgramInfoLog(res.glhandle, (gl43::GLsizei)info.length(), &size, (gl43::GLchar*)info.data());
      logError("Program failed to link: {}", info);
      for (uint32_t i = 0; i < ShaderTypeCount; ++i)
      {
        if (res.shaders[i])
          gl43::glDeleteShader(res.shaders[i]);
      }
      destroy(GfxProgram::handle(h));
      h = {};
      return h;
    }
    gl43::glValidateProgram(res.glhandle);
    gl43::glGetProgramiv(res.glhandle, gl43::GL_VALIDATE_STATUS, &status);
    if (status != gl43::GL_TRUE)
    {
      logError("Program validation failed: {}", (uint32_t)h);
      destroy(GfxProgram::handle(h));
      h = {};
    }
  }
  return h;
}

void GfxDevice43::postProcessDraw(GfxMaterial2 const& material)
{
  gl43::glUseProgram(resources.programs[material.program].glhandle);
  apply(material.layout, material.ubo, material.bindings);
  if (!fullscreenVAO)
    gl43::glGenVertexArrays(1, &fullscreenVAO);
  gl43::glBindVertexArray(fullscreenVAO);
  gl43::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
}

void GfxDevice43::beginFrame()
{
  frame++;
}

void GfxDevice43::endFrame()
{
  resources.activeResidents.erase(std::remove_if(resources.activeResidents.begin(), resources.activeResidents.end(),
                                                 [this](auto hdl)
                                                 {
                                                   auto& bhdl = resources.bindlessHandles[hdl];
                                                   if (!bhdl.resident)
                                                   {
                                                     bhdl.active = false;
                                                     return true;
                                                   }
                                                   if (bhdl.residentFrame < this->frame - 1)
                                                   {
                                                     if (bhdl.type == BindlessHandleType::eTexture)
                                                       gl43ext::glMakeTextureHandleNonResidentARB(bhdl.hdev);
                                                     else
                                                       gl43ext::glMakeImageHandleNonResidentARB(bhdl.hdev);
                                                     // are there any bhdl handles same as me
                                                     // resources.bindlessHandles.for_each(
                                                     //   [&bhdl](auto& handle)
                                                     //   {
                                                     //     if (handle.hdev == bhdl.hdev)
                                                     //       handle.resident = false;
                                                     //     return true;
                                                     //   });
                                                     bhdl.resident = false;
                                                     bhdl.active   = false;
                                                     return true;
                                                   }
                                                   return false;
                                                 }),
                                  resources.activeResidents.end());
  gl43::glDeleteBuffers((gl::GLsizei)uboList.size(), uboList.data());
  uboList.clear();
}

GfxPass::handle GfxDevice43::createPass(std::span<GfxPass::Attachment> colors, GfxPass::Attachment depth)
{
  auto       h    = resources.passes.emplace();
  GfxPassGl& pass = resources.passes.at(h);

  gl43::glGenFramebuffers(1, &pass.glhandle);
  gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, pass.glhandle);
  pass.nbImages = (uint32_t)colors.size();
  for (uint32_t i = 0; i < pass.nbImages; ++i)
  {
    auto& image              = resources.images[colors[i].image];
    pass.images[i]           = colors[i].image;
    pass.imageClearColors[i] = colors[i].colorVal;
    pass.imageClears[i]      = colors[i].clear;
    image.ref++;
    gl43::glFramebufferTexture2D(gl43::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, gl::GL_TEXTURE_2D, image.glhandle,
                                 0);
    if (gl43::glCheckFramebufferStatus(gl43::GL_FRAMEBUFFER) != gl43::GL_FRAMEBUFFER_COMPLETE)
    {
      logError("Error in creating framebuffer.");
      break;
    }
  }

  if (pass.depth = depth.image)
  {
    auto& image          = resources.images[depth.image];
    pass.depth           = depth.image;
    pass.depthClearValue = depth.depthVal;
    pass.depthClear      = depth.clear;
    gl43::glFramebufferTexture2D(gl43::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, gl::GL_TEXTURE_2D, image.glhandle, 0);
    image.ref++;
  }

  if (gl43::glCheckFramebufferStatus(gl43::GL_FRAMEBUFFER) != gl43::GL_FRAMEBUFFER_COMPLETE)
  {
    logError("Error in creating framebuffer.");
    gl43::glDeleteFramebuffers(1, &pass.glhandle);
    pass.glhandle = 0;
  }
  return h;
}

void GfxDevice43::destroy(GfxPass::handle h)
{
  GfxPassGl& pass = resources.passes.at(h);
  for (uint32_t i = 0; i < pass.nbImages; ++i)
    releaseTexture(pass.images[i]);
  releaseTexture(pass.depth);
  if (pass.glhandle)
    gl43::glDeleteFramebuffers(1, &pass.glhandle);
  resources.passes.erase(h);
}

void GfxDevice43::blit(GfxImage::handle src, GfxImage::handle dst, Rect const& srcZone, Rect const& dstZone)
{
  auto& srcimg = resources.images[src];

  if (!srcimg.fbo)
  {
    gl43::glGenFramebuffers(1, &srcimg.fbo);
    gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, srcimg.fbo);
    gl43::glFramebufferTexture2D(gl43::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, gl::GL_TEXTURE_2D,
                                 resources.images[src].glhandle, 0);
    if (gl43::glCheckFramebufferStatus(gl43::GL_FRAMEBUFFER) != gl43::GL_FRAMEBUFFER_COMPLETE)
    {
      logError("Error in creating framebuffer.");
      return;
    }
    gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
  }

  gl43::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, srcimg.fbo);

  if (dst)
  {
    auto& dstimg = resources.images[dst];
    if (!dstimg.fbo)
    {
      gl43::glGenFramebuffers(1, &dstimg.fbo);
      gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, dstimg.fbo);
      gl43::glFramebufferTexture2D(gl43::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, gl::GL_TEXTURE_2D,
                                   resources.images[dst].glhandle, 0);
      gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
      if (gl43::glCheckFramebufferStatus(gl43::GL_FRAMEBUFFER) != gl43::GL_FRAMEBUFFER_COMPLETE)
      {
        logError("Error in creating framebuffer.");
        return;
      }
    }
    gl43::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, dstimg.fbo);
  }
  else
    gl43::glDrawBuffer(gl::GL_BACK_LEFT);
  gl43::glBlitFramebuffer(srcZone.offset.x, srcZone.offset.y, srcZone.offset.x + (int)srcZone.size.x,
                          srcZone.offset.y + (int)srcZone.size.y, dstZone.offset.x, dstZone.offset.y,
                          dstZone.offset.x + (int)dstZone.size.x, dstZone.offset.y + (int)dstZone.size.y,
                          gl::GL_COLOR_BUFFER_BIT, gl::GL_NEAREST);
  gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
  gl43::glDrawBuffer(gl::GL_BACK_LEFT);
  gl43::glReadBuffer(gl::GL_BACK_LEFT);
}

void GfxDevice43::beginPass(GfxPass::handle h)
{
  if (!h)
  {
    gl43::glDrawBuffer(gl::GL_BACK_LEFT);
    gl43::glReadBuffer(gl::GL_BACK_LEFT);
  }
  else
  {
    auto& pass = resources.passes[h];
    gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, pass.glhandle);
    flags |= HasFramebuffer;
    gl43::GLenum buffers[8];
    for (uint32_t i = 0; i < pass.nbImages; ++i)
      buffers[i] = gl43::GL_COLOR_ATTACHMENT0 + i;
    gl43::glDrawBuffers(pass.nbImages, buffers);
    for (uint32_t i = 0; i < pass.nbImages; ++i)
    {
      if (pass.imageClears[i])
        gl43::glClearBufferfv(gl::GL_COLOR, i, &pass.imageClearColors[i].x);
    }
    gl43::glClearBufferfv(gl::GL_DEPTH, 0, &pass.depthClearValue);
  }
}

void GfxDevice43::endPass()
{
  if (flags & HasFramebuffer)
  {
    gl43::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    flags &= ~HasFramebuffer;
  }
}
} // namespace terra
