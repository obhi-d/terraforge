
#pragma once

#include "RenderResource.h"
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>

namespace glb = glbinding;
namespace terra
{
using tgl = glbinding::Binding;

enum class GfxVertexFormat
{
  eFloat,
  eFloat2, // 2 float
  eFloat3, // 3 float
  eUint32,
  eUnormUint32,
  eUnormByte4
};

struct GfxMesh
{
  using handle = terra::handle<GfxMesh>;

  enum Type
  {
    eLines,
    eTriangles,
    ePoints
  };

  struct Buffer
  {
    GfxBuffer::handle handle;
    uint32_t          offset = 0;
    uint32_t          size   = 0;
  };

  struct VertexBuffer
  {
    uint32_t          stride = 0;
    uint32_t          formatCount = 0;
    GfxVertexFormat   formats[4]  = {};
    uint32_t          shaderBinding[4] = {};
  };

  struct Layout
  {
    VertexBuffer vertexBuffers[4];
    uint32_t     indexBufferStride = 0;
    uint32_t     vertexBufferCount = 0;

    inline bool operator==(Layout const& other) const noexcept 
    {
      return std::memcmp(&other, this, sizeof(other)) == 0;
    }
    inline bool operator!=(Layout const& other) const noexcept
    {
      return !(other==*this);
    }
  };

  struct LayoutHash
  {
    inline std::size_t operator()(Layout const& l) const noexcept 
    {
      return fnv1a(&l, sizeof(l));
    }
  };

  struct Draw
  {
    Buffer   vertexBuffers[4];
    Buffer   indexBuffer;
    uint32_t baseVertex  = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount  = 0;
    Type     type        = Type::eTriangles;
  };
};
} // namespace terra
