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
  eBuffer,         // storage buffer handle
  eImage,
  eConstants // ubo buffer handle
};
struct GfxDescriptorSetLayout
{
  using handle         = terra::handle<GfxDescriptorSetLayout>;
  using DescriptorType = GfxDescriptorType;
  struct Descriptor
  {
    DescriptorType type;
    int32_t        binding;
    Access         access;
  };
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

struct GfxFeature
{
  int          version                      = 450; // min version is 430
  // GlGfxSupport ARB_program_interface_query      = GlGfxSupport::eUnsupported;
  // GlGfxSupport EXT_shader_image_load_store  = GlGfxSupport::eUnsupported;
  // GlGfxSupport ARB_shading_language_420pack = GlGfxSupport::eUnsupported;
  // GlGfxSupport ARB_shader_storage_buffer_object = GlGfxSupport::eUnsupported;
  // bool std430Packing = false;
};

} // namespace terra