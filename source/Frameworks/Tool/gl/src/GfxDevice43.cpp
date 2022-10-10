
#include "GfxDevice43.h"
#include "GfxShaderBuilder.h"
#include "GlGfxUtils.h"
#include "Logger.h"
#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl43core/gl.h>
#ifdef _WIN32
extern "C"
{
  _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

namespace terra
{
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
  features.version = (int)version.majorVersion() * 100 + (int)version.minorVersion() * 10;
  #ifndef NDEBUG
  gl43::glEnable(gl43::GL_DEBUG_OUTPUT);
  gl43::glDebugMessageCallback(MessageCallback, nullptr);
  #endif
  logInfo("OpenGL {}.{} - {}", version.majorVersion(), version.minorVersion(), glbinding::aux::ContextInfo::vendor());
}

void GfxDevice43::clearBackbuffer(glm::vec4 color)
{
  gl43::glClearColor(color.r, color.g, color.b, color.a);
  gl43::glClear(gl43::GL_COLOR_BUFFER_BIT);
}

void GfxDevice43::flushStates() 
{
  state.flush = true;
}
void GfxDevice43::setState(GlGfxState const& newState)
{
  if (newState.blend != state.blend || state.flush)
  {
    switch (newState.blend)
    {
    case BlendMode::eDisabled:
      gl43::glDisable(gl43::GL_BLEND);
      break;
    case BlendMode::eAdditive:
      gl43::glEnable(gl43::GL_BLEND);
      gl43::glBlendEquation(gl43::GL_FUNC_ADD);
      gl43::glBlendFuncSeparate(gl43::GL_SRC_ALPHA, gl43::GL_ONE_MINUS_SRC_ALPHA, gl43::GL_ONE,
                                gl43::GL_ONE_MINUS_SRC_ALPHA);
      break;
    }
    state.blend = newState.blend;
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
    case DepthTestMode::eGreaterEq:
      gl43::glEnable(gl43::GL_DEPTH_TEST);
      gl43::glDepthFunc(gl43::GL_GEQUAL);
      break;
    }
    state.depthTest = newState.depthTest;
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
  if (state.scissorsEnabled && (newState.scissor != state.scissor|| state.flush))
  {
    gl43::glScissor(newState.scissor.offset.x, newState.scissor.offset.y, newState.scissor.size.x,
                    newState.scissor.size.y);
    state.scissor = newState.scissor;
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
  return h;
}
void GfxDevice43::destroy(GfxBuffer::handle h)
{
  if (!h)
    return;
  auto& res = resources.buffers.at(h);
  gl43::glDeleteBuffers(1, &res.glhandle);
  resources.buffers.erase(h);
}
GfxImage2D::handle GfxDevice43::createImage(GfxStorageClass storage, uint32_t width, uint32_t height,
                                            ImageFormat format, std::byte const* data, GfxImage2D::Swizzle swizzle,
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
  gl43::glTexParameteri(gl43::GL_TEXTURE_2D, gl43::GL_TEXTURE_SWIZZLE_R, toGl(swizzle.r));
  gl43::glTexParameteri(gl43::GL_TEXTURE_2D, gl43::GL_TEXTURE_SWIZZLE_G, toGl(swizzle.g));
  gl43::glTexParameteri(gl43::GL_TEXTURE_2D, gl43::GL_TEXTURE_SWIZZLE_B, toGl(swizzle.b));
  gl43::glTexParameteri(gl43::GL_TEXTURE_2D, gl43::GL_TEXTURE_SWIZZLE_A, toGl(swizzle.a));
  if (mipLevels > 1)
  {
    gl43::glGenerateMipmap(gl43::GL_TEXTURE_2D);
  }
  return h;
}
void GfxDevice43::destroy(GfxImage2D::handle h)
{
  if (!h)
    return;

  auto& res = resources.images.at(h);
  gl43::glDeleteTextures(1, &res.glhandle);
  resources.images.erase(h);
}
GfxSampler::handle GfxDevice43::createSampler(ImageSampling sampling)
{
  auto  h   = resources.samplers.emplace();
  auto& res = resources.samplers.at(h);
  gl43::glGenSamplers(1, &res.glhandle);
  {
    gl43::GLenum minFilter = gl43::GLenum::GL_NEAREST_MIPMAP_NEAREST;
    gl43::GLenum magFilter = gl43::GLenum::GL_NEAREST;
    switch (sampling.first)
    {
    case SamplingType::eLinear:
      minFilter = gl43::GLenum::GL_LINEAR_MIPMAP_LINEAR;
      magFilter = gl43::GLenum::GL_LINEAR;
      break;
    }

    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_MIN_FILTER, minFilter);
    gl43::glSamplerParameteri(res.glhandle, gl43::GL_TEXTURE_MAG_FILTER, magFilter);
  }
  {
    gl43::GLenum tiling = gl43::GLenum::GL_REPEAT;
    switch (sampling.second)
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

  gl43::glShaderSource(glhandle, (gl43::GLsizei)sources.size(), sources.data(), lengths.data());
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
GfxProgram::handle GfxDevice43::createProgram(ShaderOptions const& options, ShaderBuilder const& sources)
{
  std::string optionStr = std::format("#version {}\n", this->features.version);
  for (uint32_t i = 0; i < options.names.size(); ++i)
  {
    std::format_to(std::back_inserter(optionStr), "#define {} {}\n", options.names[i],
                   (uint32_t)((options.bitMask & (1ull << i)) != 0));
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
  auto  h   = resources.meshes.emplace();
  auto& res = resources.meshes.at(h);
  auto  it  = resources.meshMap.emplace(mesh, h);
  res.desc  = &it.first->first;
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
std::byte* GfxDevice43::mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size)
{
  auto& res = resources.buffers.at(buffer);
  assert(offset + size <= res.size);
  gl43::glBindBuffer(res.target, res.glhandle);
  return (std::byte*)gl43::glMapBufferRange(res.target, offset, size,
                                            gl43::MapBufferAccessMask::GL_MAP_WRITE_BIT |
                                              gl43::MapBufferAccessMask::GL_MAP_INVALIDATE_RANGE_BIT);
}
void GfxDevice43::unmapBuffer(GfxBuffer::handle buffer)
{
  auto& res = resources.buffers.at(buffer);
  gl43::glUnmapBuffer(res.target);
}
void GfxDevice43::updateImage(GfxImage2D::handle image, std::span<std::byte const> data)
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
void GfxDevice43::readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<std::byte> out)
{
  auto& res = resources.buffers.at(buffer);
  gl43::glBindBuffer(res.target, res.glhandle);
  gl43::glGetBufferSubData(res.target, offset, out.size_bytes(), out.data());
}
void GfxDevice43::readImage(GfxImage2D::handle buffer, std::span<std::byte> out)
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
void GfxDevice43::draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& mat)
{
  gl43::glUseProgram(resources.programs[mat.program].glhandle);
  bindResources(mat.descriptorSet);
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
      gl43::glBindSampler(layout.descriptors[i].binding, resources.samplers[res.values[i].second].glhandle);
      gl43::glActiveTexture(gl43::GL_TEXTURE0 + layout.descriptors[i].binding);
      gl43::glBindTexture(gl43::GL_TEXTURE_2D, resources.images[res.values[i].first].glhandle);
      break;
    }
  }
}
void GfxDevice43::applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle layout) {}
} // namespace terra