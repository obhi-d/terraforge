
#include "GfxDevice45.h"
#include "GfxShaderBuilder.h"
#include "GlGfxUtils.h"
#include "Logger.h"
#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl45core/gl.h>

namespace terra
{
namespace gl45 = gl45core;
GfxBuffer::handle GfxDevice45::createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size)
{
  auto  h     = resources.buffers.emplace();
  auto& res   = resources.buffers.at(h);
  res.usage   = usage;
  res.size    = size;
  res.storage = storage;
  gl45::glCreateBuffers(1, &res.glhandle);
  gl45::BufferStorageMask storageMask = gl45::BufferStorageMask::GL_NONE_BIT;
  switch (storage)
  {
  case GfxStorageClass::eDynamicDeviceAccess:
  case GfxStorageClass::eDeviceAccess:
    break;
  case GfxStorageClass::eReadback:
    break;
  case GfxStorageClass::eDynamicDeviceReadonly:
    storageMask |= gl45::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT;
    [[fallthrough]];
  case GfxStorageClass::eStaticDeviceReadonly:
    storageMask |= gl45::BufferStorageMask::GL_MAP_WRITE_BIT;
    break;
  }
  gl45::glNamedBufferStorage(res.glhandle, size, nullptr, storageMask);
  return h;
}

GfxImage2D::handle GfxDevice45::createImage(GfxStorageClass storage, uint32_t width, uint32_t height,
                                            ImageFormat format, ubyte_t const* data, GfxImage2D::Swizzle swizzle,
                                            uint32 mipLevels)
{
  auto  h     = resources.images.emplace();
  auto& res   = resources.images.at(h);
  res.width   = width;
  res.height  = height;
  res.format  = format;
  res.storage = storage;
  gl45::glGenTextures(1, &res.glhandle);

  // For some reason, intel driver fails without bind texture
  gl45::glActiveTexture(gl45::GL_TEXTURE0);
  gl45::glBindTexture(gl45::GL_TEXTURE_2D, res.glhandle);

  gl45::glTextureStorage2D(res.glhandle, 1, toGlFormat(format), width, height);
  gl45::glTextureSubImage2D(res.glhandle, 0, 0, 0, width, height, toGlDataFormat(format), toGlType(format), data);
  gl45::glTextureParameteri(res.glhandle, gl45::GL_TEXTURE_SWIZZLE_R, toGl(swizzle.r));
  gl45::glTextureParameteri(res.glhandle, gl45::GL_TEXTURE_SWIZZLE_G, toGl(swizzle.g));
  gl45::glTextureParameteri(res.glhandle, gl45::GL_TEXTURE_SWIZZLE_B, toGl(swizzle.b));
  gl45::glTextureParameteri(res.glhandle, gl45::GL_TEXTURE_SWIZZLE_A, toGl(swizzle.a));
  if (mipLevels > 1)
    gl45::glGenerateTextureMipmap(res.glhandle);
  return h;
}

GfxMesh::handle GfxDevice45::createMeshLayout(GfxMesh::Layout const& mesh)
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
  gl45::glCreateVertexArrays(1, &res.glhandle);  
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
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl45::GL_FLOAT, gl45::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eFloat2:
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 2, gl45::GL_FLOAT, gl45::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eFloat3:
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 3, gl45::GL_FLOAT, gl45::GL_FALSE, el.relOffset);
          break;
        case GfxVertexFormat::eUint32:
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl45::GL_UNSIGNED_INT, gl45::GL_FALSE,
                                          el.relOffset);
          break;
        case GfxVertexFormat::eUnormUint32:
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 1, gl45::GL_UNSIGNED_INT, gl45::GL_TRUE,
                                          el.relOffset);
          break;
        case GfxVertexFormat::eUnormByte4:
          gl45::glVertexArrayAttribFormat(res.glhandle, attribCount, 4, gl45::GL_UNSIGNED_BYTE, gl45::GL_TRUE,
                                          el.relOffset);
          break;
        }

        gl45::glVertexArrayAttribBinding(res.glhandle, attribCount, i);
        gl45::glEnableVertexArrayAttrib(res.glhandle, attribCount);
      }
      else
        gl45::glDisableVertexArrayAttrib(res.glhandle, -el.shaderBinding);
    }
  }
  return GfxMesh::handle(h);
}

ubyte_t* GfxDevice45::mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size)
{
  auto& res = resources.buffers.at(buffer);
  assert(offset + size <= res.size);
  return (ubyte_t*)gl45::glMapNamedBufferRange(res.glhandle, offset, size,
                                                 gl45::MapBufferAccessMask::GL_MAP_WRITE_BIT |
                                                   gl45::MapBufferAccessMask::GL_MAP_INVALIDATE_RANGE_BIT);
}
void GfxDevice45::unmapBuffer(GfxBuffer::handle buffer)
{
  auto& res = resources.buffers.at(buffer);
  gl45::glUnmapNamedBuffer(res.glhandle);
}
void GfxDevice45::updateImage(GfxImage2D::handle image, std::span<ubyte_t const> data)
{
  auto& res = resources.images.at(image);
  gl45::glTextureSubImage2D(res.glhandle, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format),
                            toGlType(res.format), data.data());
}

void GfxDevice45::readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out)
{
  auto& res = resources.buffers.at(buffer);
  gl45::glGetNamedBufferSubData(res.glhandle, offset, out.size_bytes(), out.data());
}
void GfxDevice45::readImage(GfxImage2D::handle buffer, std::span<ubyte_t> out)
{
  auto& res = resources.images.at(buffer);
  gl45::glTextureSubImage2D(res.glhandle, 0, 0, 0, res.width, res.height, toGlDataFormat(res.format),
                            toGlType(res.format), out.data());
}

void GfxDevice45::draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& mat)
{
  gl45::glUseProgram(resources.programs[mat.program].glhandle);
  bindResources(mat.descriptorSet);
  auto& mesh = resources.meshes[drawDesc.layout];
  
  for (uint32_t i = 0; i < mesh.desc->vertexBufferCount; ++i)
  {
    auto bufferOffset = drawDesc.vertexBuffers[i].offset;
    if (mesh.vertexBuffers[i] != drawDesc.vertexBuffers[i].handle || bufferOffset != mesh.vertexBufferOffsets[i])
    {
      auto oglBuffer = resources.buffers[drawDesc.vertexBuffers[i].handle].glhandle;
      gl45::glVertexArrayVertexBuffer(mesh.glhandle, i, oglBuffer, bufferOffset, mesh.desc->vertexBuffers[i].stride);
      mesh.vertexBuffers[i]       = drawDesc.vertexBuffers[i].handle;
      mesh.vertexBufferOffsets[i] = bufferOffset;
    }
  }

  auto mode = toGLDrawMode(drawDesc.type);
  if (drawDesc.indexCount > 0)
  {
    if (mesh.elementBuffer != drawDesc.indexBuffer.handle)
    {
      auto indexBuffer = resources.buffers[drawDesc.indexBuffer.handle].glhandle;
      gl45::glVertexArrayElementBuffer(mesh.glhandle, indexBuffer);
      mesh.elementBuffer = drawDesc.indexBuffer.handle;
    }
    gl45::glBindVertexArray(mesh.glhandle);  
    if (drawDesc.baseVertex == 0)
      gl45::glDrawElements(mode, drawDesc.indexCount,
                           drawDesc.indexBufferStride == 2 ? gl45::GL_UNSIGNED_SHORT : gl45::GL_UNSIGNED_INT,
                           (void const*)(uintptr_t)drawDesc.indexBuffer.offset);
    else
      gl45::glDrawElementsBaseVertex(mode, drawDesc.indexCount,
                                     drawDesc.indexBufferStride == 2 ? gl45::GL_UNSIGNED_SHORT : gl45::GL_UNSIGNED_INT,
                                     (void const*)(uintptr_t)drawDesc.indexBuffer.offset, drawDesc.baseVertex);
  }
  else
    gl45::glDrawArrays(mode, 0, drawDesc.vertexCount);
}
} // namespace terra