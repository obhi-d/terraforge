
#include "GfxDevice.h"
#include "GlGfxUtils.h"
#include "Logger.h"
#include "GfxShaderBuilder.h"
#include <glbinding-aux/ContextInfo.h>

namespace terra
{
void GfxDevice::init() 
{
  // auto ext = glbinding::aux::ContextInfo::extensions();
  auto version = glbinding::aux::ContextInfo::version();
  if (version.majorVersion() < 4 || (version.majorVersion() == 4 && version.minorVersion() < 3))
  {
    logError("OpenGL version not supported : {}", version.toString());
    throw std::runtime_error("Failed to initalize device");
  }
  features.version   = (int)version.majorVersion() * 100 + (int)version.minorVersion() * 10; 
 }

GfxBuffer::handle GfxDevice::createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size)
{
  auto  h     = resources.buffers.emplace();
  auto& res   = resources.buffers.at(h);
  res.usage   = usage;
  res.size    = size;
  res.storage = storage;
  gl::glCreateBuffers(1, &res.glhandle);
  gl::BufferStorageMask storageMask = gl::BufferStorageMask::GL_NONE_BIT;
  switch (storage)
  {
  case GfxStorageClass::eDynamicDeviceAccess:
  case GfxStorageClass::eDeviceAccess:
    break;
  case GfxStorageClass::eReadback:
    break;
  case GfxStorageClass::eDynamicDeviceReadonly:
    storageMask |= gl::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT;
    [[fallthrough]];
  case GfxStorageClass::eStaticDeviceReadonly:
    storageMask |= gl::BufferStorageMask::GL_MAP_WRITE_BIT;
    break;
  }
  gl::glNamedBufferStorage(res.glhandle, size, nullptr, storageMask);
  return h;
}
void GfxDevice::destroy(GfxBuffer::handle h)
{
  auto& res = resources.buffers.at(h);
  gl::glDeleteBuffers(1, &res.glhandle);
  resources.buffers.erase(h);
}
GfxImage2D::handle GfxDevice::createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                          std::byte const* data)
{
  auto  h     = resources.images.emplace();
  auto& res   = resources.images.at(h);
  res.width   = width;
  res.height  = height;
  res.format  = format;
  res.storage = storage;
  gl::glGenTextures(1, &res.glhandle);
  gl::glTextureStorage2D(res.glhandle, 1, toGlFormat(format), width, height);
  gl::glTextureSubImage2D(res.glhandle, 0, 0, 0, width, height, toGlDataFormat(format), toGlType(format), data);
  return h;
}
void GfxDevice::destroy(GfxImage2D::handle h)
{
  auto& res = resources.images.at(h);
  gl::glDeleteTextures(1, &res.glhandle);
  resources.images.erase(h);
}
GfxSampler::handle GfxDevice::createSampler(ImageSampling sampling)
{
  auto  h   = resources.samplers.emplace();
  auto& res = resources.samplers.at(h);
  gl::glGenSamplers(1, &res.glhandle);
  {
    gl::GLenum minFilter = gl::GLenum::GL_NEAREST_MIPMAP_NEAREST;
    gl::GLenum magFilter = gl::GLenum::GL_NEAREST;
    switch (sampling.first)
    {
    case SamplingType::eLinear:
      minFilter = gl::GLenum::GL_LINEAR_MIPMAP_LINEAR;
      magFilter = gl::GLenum::GL_LINEAR;
      break;
    }

    gl::glSamplerParameteri(res.glhandle, gl::GL_TEXTURE_MIN_FILTER, minFilter);
    gl::glSamplerParameteri(res.glhandle, gl::GL_TEXTURE_MAG_FILTER, magFilter);
  }
  {
    gl::GLenum tiling = gl::GLenum::GL_REPEAT;
    switch (sampling.second)
    {
    case Tiling::eMirror:
      tiling = gl::GL_MIRRORED_REPEAT;
      break;
    case Tiling::eClampToEdge:
      tiling = gl::GL_CLAMP_TO_EDGE;
      break;
    }
    gl::glSamplerParameteri(res.glhandle, gl::GL_TEXTURE_WRAP_S, tiling);
    gl::glSamplerParameteri(res.glhandle, gl::GL_TEXTURE_WRAP_T, tiling);
  }
  return h;
}
void GfxDevice::destroy(GfxSampler::handle h)
{
  auto& res = resources.samplers.at(h);
  gl::glDeleteSamplers(1, &res.glhandle);
  resources.samplers.erase(h);
}
gl::GLuint GfxDevice::createShader(ShaderType type, std::span<gl::GLchar const*> sources,
                                          std::span<gl::GLint> lengths)
{
  gl::GLuint glhandle       = gl::glCreateShader(toGlType(type));


  gl::glShaderSource(glhandle, (gl::GLsizei)sources.size(), sources.data(), lengths.data());
  gl::glCompileShader(glhandle);
  gl::GLint status = {};
  gl::glGetShaderiv(glhandle, gl::GL_COMPILE_STATUS, &status);
  if (status != gl::GL_TRUE)
  {
    gl::glGetShaderiv(glhandle, gl::GL_INFO_LOG_LENGTH, &status);
    std::string info(status, 0);
    gl::GLsizei size = {};
    gl::glGetShaderInfoLog(glhandle, (gl::GLsizei)info.length(), &size, (gl::GLchar*)info.data());
    logError("Shader failed to compile: {}", info);
    gl::glDeleteShader(glhandle);
  }
  return glhandle;
}
GfxDescriptorSetLayout::handle GfxDevice::createDescriptorSetLayout(
  std::span<GfxDescriptorSetLayout::Descriptor> descriptors)
{
  auto  h             = resources.descriptorSetLayouts.emplace();
  auto& res           = resources.descriptorSetLayouts.at(h);
  res.descriptors     = std::make_unique<GfxDescriptorSetLayout::Descriptor[]>(descriptors.size());
  res.descriptorCount = (uint32_t)descriptors.size();
  std::memcpy(res.descriptors.get(), descriptors.data(), descriptors.size_bytes());
  return h;
}
void GfxDevice::destroy(GfxDescriptorSetLayout::handle h)
{
  resources.descriptorSetLayouts.erase(h);
}
GfxDescriptorSet::handle GfxDevice::createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout)
{
  auto  h      = resources.descriptorSets.emplace();
  auto& res    = resources.descriptorSets.at(h);
  auto& layout = resources.descriptorSetLayouts.at(descriptorLayout);
  res.layout   = descriptorLayout;
  res.values   = std::make_unique<GfxDescriptorSet::rhandle[]>(layout.descriptorCount);
  std::memset(res.values.get(), 0, layout.descriptorCount * sizeof(GfxDescriptorSet::rhandle));
  return h;
}
void GfxDevice::destroy(GfxDescriptorSet::handle h)
{
  resources.descriptorSets.erase(h);
}
GfxFence::handle GfxDevice::createFence()
{
  auto  h   = resources.fences.emplace();
  auto& res = resources.fences.at(h);
  res.sync  = gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  return GfxFence::handle(h);
}
void GfxDevice::syncFence(GfxFence::handle h)
{
  auto& res = resources.fences.at(h);
  gl::glWaitSync(res.sync, 0, gl::GL_TIMEOUT_IGNORED);
  resources.fences.erase(h);
}
GfxProgram::handle GfxDevice::createProgram(ShaderOptions const& options, ShaderBuilder const& sources)
{
  std::string optionStr = std::format("#version {}\n", this->features.version);
  for (uint32_t i = 0; i < options.names.size(); ++i)
  {
    std::format_to(std::back_inserter(optionStr), "#define {} {}\n", options.names[i],
                   (uint32_t)((options.bitMask & (1ull << i)) != 0));
  }

  auto&                          sb = (GfxShaderBuilder&)sources;
  std::vector<gl::GLchar const*> sourceFiles;
  std::vector<gl::GLint> sourceLengths;

  sourceFiles.emplace_back((gl::GLchar const*)optionStr.c_str());
  sourceLengths.emplace_back((gl::GLint)optionStr.length());

  sourceFiles.emplace_back((gl::GLchar const*)sb.declarations.c_str());
  sourceLengths.emplace_back((gl::GLint)sb.declarations.length());

  GfxProgram::handle h = resources.programs.emplace();
  auto& res    = resources.programs.at(h);
  res.glhandle = gl::glCreateProgram();
  for (uint32_t i = 0; i < ShaderTypeCount; ++i)
  {
    if (!sb.output[i].empty())
    {
      sourceFiles.emplace_back((gl::GLchar const*)sb.output[i].c_str());
      sourceLengths.emplace_back((gl::GLint)sb.output[i].length());
      res.shaders[i] = createShader((ShaderType)i, sourceFiles, sourceLengths);
      if (res.shaders[i])
        gl::glAttachShader(res.glhandle, res.shaders[i]);
    }
  }
  gl::glLinkProgram(res.glhandle);
  gl::GLint status = {};
  gl::glGetProgramiv(res.glhandle, gl::GL_LINK_STATUS, &status);
  if (status != gl::GL_TRUE)
  {
    gl::glGetProgramiv(res.glhandle, gl::GL_INFO_LOG_LENGTH, &status);
    std::string info(status, 0);
    gl::GLsizei size = {};
    gl::glGetProgramInfoLog(res.glhandle, (gl::GLsizei)info.length(), &size, (gl::GLchar*)info.data());
    logError("Program failed to link: {}", info);
    destroy(GfxProgram::handle(h));
    h = {};
  }
  gl::glValidateProgram(res.glhandle);
  gl::glGetProgramiv(res.glhandle, gl::GL_VALIDATE_STATUS, &status);
  if (status != gl::GL_TRUE)
  {
    logError("Program validation failed: {}", (uint32_t)h);
    destroy(GfxProgram::handle(h));
    h = {};
  }
  return h;
}
void GfxDevice::destroy(GfxProgram::handle h)
{
  auto& res = resources.programs.at(h);
  for (uint32_t i = 0; i < ShaderTypeCount; ++i)
  {
    if (res.shaders[i])
      gl::glDeleteShader(res.shaders[i]);
  }
  gl::glDeleteProgram(res.glhandle);
  resources.programs.erase(h);
}
GfxMesh::handle GfxDevice::createMeshLayout(GfxMesh::Layout const& mesh)
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
  gl::glCreateVertexArrays(1, &res.glhandle);
  uint32_t attribCount = 0;
  for (uint32_t i = 0; i < mesh.vertexBufferCount; ++i)
  {
    uint32_t relOffset = 0;
    for (uint32_t s = 0; s < mesh.vertexBuffers[i].formatCount; ++s)
    {
      switch (mesh.vertexBuffers[i].formats[s])
      {
      case GfxVertexFormat::eFloat:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl::GL_FLOAT, gl::GL_FALSE, relOffset);
        relOffset += 4;
        break;
      case GfxVertexFormat::eFloat2:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 2, gl::GL_FLOAT, gl::GL_FALSE, relOffset);
        relOffset += 8;
        break;
      case GfxVertexFormat::eFloat3:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 3, gl::GL_FLOAT, gl::GL_FALSE, relOffset);
        relOffset += 12;
        break;
      case GfxVertexFormat::eUint32:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl::GL_UNSIGNED_INT, gl::GL_FALSE, relOffset);
        relOffset += 4;
        break;
      case GfxVertexFormat::eUnormUint32:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl::GL_UNSIGNED_INT, gl::GL_TRUE, relOffset);
        relOffset += 4;
        break;
      case GfxVertexFormat::eUnormByte4:
        gl::glVertexArrayAttribFormat(res.glhandle, attribCount, 4, gl::GL_UNSIGNED_BYTE, gl::GL_TRUE, relOffset);
        relOffset += 4;
        break;
      }
      gl::glVertexArrayAttribBinding(res.glhandle, attribCount, mesh.vertexBuffers[i].shaderBinding[s]);
    }
  }
  return GfxMesh::handle(h);
}
void GfxDevice::destroy(GfxMesh::handle h)
{
  auto& res = resources.meshes.at(h);
  if (!--res.usageCounter)
  {
    resources.meshMap.erase(*res.desc);
    gl::glDeleteVertexArrays(1, &res.glhandle);
    resources.meshes.erase(h);
  }
}
std::byte* GfxDevice::mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size)
{
  auto& res = resources.buffers.at(buffer);
  assert(offset + size <= res.size);
  return (std::byte*)gl::glMapNamedBufferRange(res.glhandle, offset, size,
                                               gl::MapBufferAccessMask::GL_MAP_WRITE_BIT |
                                                 gl::MapBufferAccessMask::GL_MAP_INVALIDATE_RANGE_BIT);
}
void GfxDevice::unmapBuffer(GfxBuffer::handle buffer)
{
  auto& res = resources.buffers.at(buffer);
  gl::glUnmapNamedBuffer(res.glhandle);
}
void GfxDevice::updateImage(GfxImage2D::handle image, std::span<std::byte const> data) 
{
  auto& res = resources.images.at(image);
  gl::glTextureSubImage2D(res.glhandle, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format), toGlType(res.format), data.data());
}
void GfxDevice::updateDescriptorSet(GfxDescriptorSet::handle h, std::span<GfxDescriptorSet::rhandle> handles) 
{
  auto& res = resources.descriptorSets.at(h);
  std::memcpy(res.values.get(), handles.data(), handles.size_bytes());
}
void GfxDevice::readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<std::byte> out)
{
  auto& res = resources.buffers.at(buffer);
  gl::glGetNamedBufferSubData(res.glhandle, offset, out.size_bytes(), out.data());
}
void GfxDevice::readImage(GfxImage2D::handle buffer, std::span<std::byte> out)
{
  auto& res = resources.images.at(buffer);
  gl::glTextureSubImage2D(res.glhandle, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format) ,
    toGlType(res.format), out.data());
}
void GfxDevice::dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet,
                                       uint32_t numGroupX, uint32_t numGroupY)
{
  gl::glUseProgram(resources.programs[shader].glhandle);
  bindResources(descriptorSet);
  gl::glDispatchCompute(numGroupX, numGroupY, 1);
}
void GfxDevice::barrier(GfxBarrierFlags flags) 
{
  gl::MemoryBarrierMask mask = gl::MemoryBarrierMask::GL_NONE_BIT;
  if (flags & GfxBarrierFlags::fAtomicCounter)
    mask |= gl::MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fImageAccess)
    mask |= gl::MemoryBarrierMask::GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fStorageBuffer)
    mask |= gl::MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fTextureAccess)
    mask |= gl::MemoryBarrierMask::GL_TEXTURE_FETCH_BARRIER_BIT;
  if (flags & GfxBarrierFlags::fUniformBuffer)
    mask |= gl::MemoryBarrierMask::GL_UNIFORM_BARRIER_BIT;
  gl::glMemoryBarrier(mask);
}
std::shared_ptr<ShaderBuilder> GfxDevice::createShaderBuilder(ShaderLang) 
{
  return std::make_shared<GfxShaderBuilder>(features);
}
void GfxDevice::draw(GfxMesh::handle hmesh, GfxMesh::Draw const& drawDesc, GfxDescriptorSet::handle descriptorSet) 
{
  bindResources(descriptorSet);
  auto& mesh = resources.meshes[hmesh];
  gl::glBindVertexArray(mesh.glhandle);
  for (uint32_t i = 0; i < mesh.desc->vertexBufferCount; ++i)
  {
    gl::glBindVertexBuffer(i, resources.buffers[drawDesc.vertexBuffers[i].handle].glhandle, drawDesc.vertexBuffers[i].offset,
                           mesh.desc->vertexBuffers[i].stride);
  }
  gl::glBindBuffer(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, drawDesc.indexBuffer.handle);
  
  auto mode = toGLDrawMode(drawDesc.type);
  if (drawDesc.indexCount > 0)
  {
    gl::glDrawElementsBaseVertex(mode, drawDesc.indexCount,
                                 mesh.desc->indexBufferStride == 2 ? gl::GL_UNSIGNED_SHORT : gl::GL_UNSIGNED_INT,
                                 (void const*)(uintptr_t)drawDesc.indexBuffer.offset, drawDesc.baseVertex);
  }
  else
    gl::glDrawArrays(mode, 0, drawDesc.vertexCount);
}
void GfxDevice::bindResources(GfxDescriptorSet::handle descriptorSet) 
{
  auto& res = resources.descriptorSets[descriptorSet];
  auto& layout = resources.descriptorSetLayouts[res.layout];
  for (uint32_t i = 0; i < layout.descriptorCount; ++i)
  {
    switch (layout.descriptors[i].type)
    {
    case GfxDescriptorType::eBuffer:
      gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, layout.descriptors[i].binding, 
        resources.buffers[res.values[i].first].glhandle);
      break;
    case GfxDescriptorType::eConstants:
      gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, layout.descriptors[i].binding,
                           resources.buffers[res.values[i].first].glhandle);
      break;
    case GfxDescriptorType::eImage:
      gl::glBindImageTexture(layout.descriptors[i].binding, resources.images[res.values[i].first].glhandle, 0,
                             gl::GL_FALSE, 0, toGlMapBit(layout.descriptors[i].access), gl::GL_RG32F);
      break;
    }   
  }
}
void GfxDevice::applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle layout) 
{
}
} // namespace terra