#pragma once

#include "Common.h"
#include <cstdint>
#include <limits>
#include <string_view>
#include <variant>

namespace terra
{

enum class Access
{
  eReadWrite,
  eReadonly,
  eWriteonly
};

enum class ImageFormat
{
  eFloat,
  eUnorm8,
  eSnorm16,
  eUnorm16,
  eRgba8,
  eSrgb8Alpha8
};

enum class SamplingType
{
  eLinear,
  eNearest
};

enum class Tiling
{
  eRepeat,
  eMirror,
  eClampToEdge
};

using ImageSampling = std::pair<SamplingType, Tiling>;

enum class GfxStorageClass
{
  eReadback,
  eDeviceAccess, // gpu read write
  eDynamicDeviceAccess,
  eDynamicDeviceReadonly,
  eStaticDeviceReadonly
};

enum GfxBarrierFlags
{
  fUniformBuffer = 1 << 0,
  fAtomicCounter = 1 << 1,
  fStorageBuffer = 1 << 2,
  fImageAccess   = 1 << 3,
  fTextureAccess = 1 << 4,
  fFullBarrier   = fUniformBuffer | fAtomicCounter | fStorageBuffer | fImageAccess | fTextureAccess
};

struct GfxBuffer
{
  using handle = terra::handle<GfxBuffer>;

  enum Usage
  {
    fUniform = 1 << 0,
    fStorage = 1 << 1,
    fVertex  = 1 << 2,
    fIndex   = 1 << 3
  };
};

struct GfxImage2D
{
  enum ComponentValue : uint8_t
  {
    eRed,
    eGreen,
    eBlue,
    eAlpha,
    eZero,
    eOne
  };

  struct Swizzle
  {
    ComponentValue r = eRed;
    ComponentValue g = eGreen;
    ComponentValue b = eBlue;
    ComponentValue a = eAlpha;
  };

  using handle = terra::handle<GfxImage2D>;
  using Format = ImageFormat;
};

struct GfxSampler
{
  using handle = terra::handle<GfxSampler>;
};

enum class ShaderLang
{
  eGLSL
};
enum class ShaderType
{
  eVertex,
  eFragment,
  eCompute,
  kCount
};
static inline constexpr uint32_t ShaderTypeCount = (uint32_t)ShaderType::kCount;

enum class GfxDescriptorType
{
  eBuffer, // storage buffer handle
  eImage,
  eTexture,
  eConstants // ubo buffer handle
};

struct GfxDescriptor
{
  GfxDescriptorType type;
  int32_t           binding;
  Access            access;
};

struct GfxDescriptorSetLayout
{
  using handle         = terra::handle<GfxDescriptorSetLayout>;
  using DescriptorType = GfxDescriptorType;
  using Descriptor     = GfxDescriptor;
};

struct GfxDescriptorSet
{
  using handle  = terra::handle<GfxDescriptorSet>;
  using rhandle = std::pair<uint32_t, uint32_t>;
};

struct GfxFence
{
  using handle = terra::handle<GfxFence>;
};

struct GfxProgram
{
  using handle = terra::handle<GfxProgram>;
};

enum class GlGfxSupport
{
  eCore,
  eSupported,
  eUnsupported
};

enum class GfxVertexFormat
{
  eFloat,
  eFloat2, // 2 float
  eFloat3, // 3 float
  eUint32,
  eUnormUint32,
  eUnormByte4
};
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

struct GfxState
{
  CullMode      cullMode        = CullMode::eCullBack;
  BlendMode     blend           = BlendMode::eDisabled;
  DepthTestMode depthTest       = DepthTestMode::eDisabled;
  bool          scissorsEnabled = false;
  Rect          viewport;
  Rect          scissor;
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

struct GfxFeature
{
  int version = 450; // min version is 430
  // GlGfxSupport ARB_program_interface_query      = GlGfxSupport::eUnsupported;
  // GlGfxSupport EXT_shader_image_load_store  = GlGfxSupport::eUnsupported;
  // GlGfxSupport ARB_shading_language_420pack = GlGfxSupport::eUnsupported;
  // GlGfxSupport ARB_shader_storage_buffer_object = GlGfxSupport::eUnsupported;
  // bool std430Packing = false;
};

inline uint32_t getMipLevels(uint32_t width, uint32_t height)
{
  uint32_t           lvl       = 1;
  constexpr uint32_t minLvlDim = 1;
  while (width > minLvlDim || height > minLvlDim)
  {
    width  = std::max(width >> 1, minLvlDim);
    height = std::max(height >> 1, minLvlDim);
    lvl++;
  }
  return lvl;
}

inline uint32_t getBaseSize(ImageFormat format)
{
  switch (format)
  {
  case ImageFormat::eSrgb8Alpha8:
  case ImageFormat::eRgba8:
  case ImageFormat::eFloat:
    return 4;
  case ImageFormat::eUnorm16:
  case ImageFormat::eSnorm16:
    return 2;
  case ImageFormat::eUnorm8:
    return 1;
  }
}
} // namespace terra