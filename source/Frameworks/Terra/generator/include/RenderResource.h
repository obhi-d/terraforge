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

enum class SamplingType
{
  eLinear,
  eTrilinear,
  eNearest
};

enum class Tiling
{
  eRepeat,
  eMirror,
  eClampToEdge
};

enum class SampleCompare
{
  eNone,
  eGEq,
  eLEq,
  eGT,
  eLT
};

enum class DepthClear : uint8_t
{
  eNone,
  eClearZ_1,
  eClearZ_0
};

struct ImageSampling
{
  SamplingType  sampling = SamplingType::eLinear;
  Tiling        tiling   = Tiling::eRepeat;
  SampleCompare compare  = SampleCompare::eNone;

  ImageSampling() = default;
  ImageSampling(SamplingType isampling, Tiling itiling = Tiling::eRepeat, SampleCompare icompare = SampleCompare::eNone)
      : sampling(isampling), tiling(itiling), compare(icompare)
  {}
};

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
  fNone          = 0,
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

struct GfxImage
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

  using handle = terra::handle<GfxImage>;
  using Format = ImageFormatEnum;
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
  eGeometry,
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

enum class GfxBindType : uint8_t
{
  eNone,
  eStorageBuffer,
  eTexture1D,
  eTexture1DArray,
  eTexture2D,
  eShadowTexture2D,
  eTextureBuffer,
  eStorageImage2D,
  eDepthBuffer,
  eUBO,
  eInt,
  eUint,
  eInt2,
  eUint2,
  eFloat,
  eFloat2,
  eFloat3,
  eFloat4,
  eMat4,
  eFloatArray,
  eIntArray,
  eUintArray
};

enum class GfxAccess : uint8_t
{
  eReadOnly,
  eWriteOnly,
  eReadWrite
};

struct StorageBuffer
{
  static inline constexpr GfxBindType type = GfxBindType::eStorageBuffer;
  GfxBuffer::handle                   buffer;
  uint32_t                            offset = 0;
  uint32_t                            size   = 0;
};

struct SampledTexture
{
  static inline constexpr std::array<GfxBindType, 4> type = {GfxBindType::eTexture2D, GfxBindType::eTexture1DArray,
                                                             GfxBindType::eShadowTexture2D, GfxBindType::eTexture1D};
  GfxImage::handle                                   texture;
  GfxSampler::handle                                 sampler;
};

struct TextureBuffer
{
  static inline constexpr GfxBindType type = GfxBindType::eTextureBuffer;
  GfxBuffer::handle                   buffer;
  ImageFormatEnum                     format = ImageFormatEnum::eFloat;
};

struct StorageImage
{
  static inline constexpr GfxBindType type = GfxBindType::eStorageImage2D;
  GfxImage::handle                    texture;
  uint16_t                            layer   = 0;
  GfxAccess                           access  = GfxAccess::eReadWrite;
  bool                                layered = false;
};

struct GfxParamLayout
{
  struct Entry
  {
    uint32_t    index = 0;
    GfxBindType type;
  };

  struct UBOEntry
  {
    std::string name;
    uint16_t    maxArraySize    = 0;
    uint16_t    baseElementSize = 0;
  };

  struct UBOOffset
  {
    uint16_t baseElementSize = 0;
    uint16_t arrayStride     = 0;
    uint32_t offset          = 0;
  };

  struct UBOReflect
  {
    std::vector<UBOOffset> offsets;
    uint32_t               uboSize = 0;
  };

  using handle = terra::handle<GfxParamLayout>;
};

struct GfxFence
{
  using handle = terra::handle<GfxFence>;
};

struct GfxProgram
{
  enum Stage
  {
    fVertex   = 1 << 0,
    fGeometry = 1 << 1,
    fFragment = 1 << 2,
    fCompute  = 1 << 3
  };

  static inline constexpr uint32_t MaxStage = 4;

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

struct GfxBlendState
{
  BlendMode mode = BlendMode::eDisabled;
};

struct GfxPass
{
  using handle = terra::handle<GfxPass>;
  struct Attachment
  {
    GfxImage::handle image;
    union
    {
      vec4  colorVal;
      float depthVal;
    };
    bool clear = false;
  };
};

struct GfxState
{
  CullMode                     cullMode  = CullMode::eCullBack;
  DepthTestMode                depthTest = DepthTestMode::eDisabled;
  std::array<GfxBlendState, 8> blend;
  float                        polyOffSlope = 1.1f;
  float                        polyOffBias  = 0.0001f;
  Rect                         viewport;
  Rect                         scissor;
  uint16_t                     nbBlendModes    = 0;
  bool                         polygonOffset   = false;
  bool                         scissorsEnabled = false;
  bool                         flush           = false;
  bool                         depthWrite      = true;
};

struct GfxMesh
{
  using handle = terra::handle<GfxMesh>;

  enum Type
  {
    eLines,
    eTriangles,
    eStrips,
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

using UboData = acl::dynamic_array<ubyte_t>;
struct GfxMaterial2
{
  GfxProgram::handle     program;
  GfxParamLayout::handle layout;
  Blob const&            bindings;
  UboData const&         ubo;

  GfxMaterial2(GfxProgram::handle iprogram, GfxParamLayout::handle ilayout, Blob const& ibindings, UboData const& iubo)
      : program(iprogram), layout(ilayout), bindings(ibindings), ubo(iubo)
  {}
  GfxMaterial2(GfxMaterial2 const& other)
      : program(other.program), layout(other.layout), bindings(other.bindings), ubo(other.ubo)
  {}
  GfxMaterial2(GfxMaterial2&& other)
      : program(other.program), layout(other.layout), bindings(other.bindings), ubo(other.ubo)
  {}
};

struct GfxFeature
{
  int          version              = 450; // min version is 430
  GlGfxSupport ARB_clip_control     = GlGfxSupport::eUnsupported;
  GlGfxSupport ARB_bindless_texture = GlGfxSupport::eUnsupported;
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

inline uint32_t getBaseSize(ImageFormatEnum format)
{
  switch (format)
  {
  case ImageFormatEnum::eSrgb8Alpha8:
  case ImageFormatEnum::eRgba8:
  case ImageFormatEnum::eFloat:
    return 4;
  case ImageFormatEnum::eUnorm16:
  case ImageFormatEnum::eSnorm16:
    return 2;
  case ImageFormatEnum::eUnorm8:
    return 1;
  }
  return 0;
}
} // namespace terra