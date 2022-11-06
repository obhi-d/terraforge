
#pragma once

#include "Setup.h"
#include "RenderResource.h"
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

namespace glb = glbinding;
namespace terra
{
using tgl = glbinding::Binding;
enum class BlendMode
{
  eDisabled,
  eAdditive
};

enum class DepthTestMode
{
  eDisabled,
  eLessEq,
  eGreaterEq
};

enum class CullMode
{
  eCullBack,
  eCullFront,
  eCullNone
};

using GlRect = Rect;

struct GlGfxState
{
  CullMode      cullMode        = CullMode::eCullBack;
  BlendMode     blend           = BlendMode::eDisabled;
  DepthTestMode depthTest       = DepthTestMode::eDisabled;
  bool          scissorsEnabled = false;
  GlRect        viewport;
  GlRect        scissor;
  bool          flush = false;
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
  };

  struct VertexElement
  {
    GfxVertexFormat format        = {};
    uint32_t        relOffset     = 0;
    int32_t         shaderBinding = -1;
  };

  struct VertexBuffer
  {
    uint32_t      stride       = 0;
    uint32_t      elementCount = 0;
    VertexElement elements[8];
  };

  struct Layout
  {
    VertexBuffer vertexBuffers[4];
    uint32_t     vertexBufferCount = 0;

    inline bool operator==(Layout const& other) const noexcept
    {
      return std::memcmp(&other, this, sizeof(other)) == 0;
    }
    inline bool operator!=(Layout const& other) const noexcept
    {
      return !(other == *this);
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
    handle   layout;
    Buffer   vertexBuffers[4];
    Buffer   indexBuffer;
    uint32_t indexBufferStride = 0;
    uint32_t baseVertex        = 0;
    uint32_t vertexCount       = 0;
    uint32_t indexCount        = 0;
    Type     type              = Type::eTriangles;
  };
};

struct GfxMaterial
{
  GfxProgram::handle       program;
  GfxDescriptorSet::handle descriptorSet;
};

} // namespace terra
