#pragma once

#include "Common.h"
#include <cstdint>
#include <limits>
#include <string_view>
#include <variant>

namespace terra
{
enum class ImageFormat
{
  eFloat,
  eUnorm16
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
  eGPUReadWrite,
  eDynamicGPUReadWrite,
  eStaticGPUReadOnly
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

  enum class Usage
  {
    eUniform,
    eStorage
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

struct GfxCompute
{
  using handle = terra::handle<GfxCompute>;
  enum class Language
  {
    eGLSL
  };
};

struct GfxDescriptorSetLayout
{
  using handle = terra::handle<GfxDescriptorSetLayout>;
  enum class DescriptorType
  {
    eReadonlyBuffer, // storage buffer handle
    eBuffer,         // storage buffer handle
    eReadonlyImage,  // image handle
    eImage,
    eConstants // ubo buffer handle
  };
  struct Descriptor
  {
    DescriptorType type;
    int32_t        binding;
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
} // namespace terra