#pragma once

#include "ImageCodec.h"
#include <cstdint>
#include <limits>
#include <string_view>

namespace terra
{

template <typename T>
struct handle
{
  handle() = default;
  handle(uint32_t v) : reserved(v) {}

  operator uint32_t() const
  {
    return reserved;
  }

public:
  uint32_t reserved = std::numeric_limits<uint32_t>::max();
};

struct GfxBuffer
{
  using handle = terra::handle<GfxBuffer>;
  enum class Storage
  {
    eDynamicGPUReadWrite,
    eStaticGPUReadOnly
  };

  enum class Usage
  {
    eUniform,
    eStorage
  };

  enum BarrierFlags
  {
    fUniformBuffer = 1 << 0,
    fAtomicCounter = 1 << 1,
    fStorageBuffer = 1 << 2
  };
};

struct GfxImage2D
{
  using handle = terra::handle<GfxImage2D>;
  using Format = ImageFormat;
};

struct GfxCompute
{
  using handle = terra::handle<GfxCompute>;
  enum class Language
  {
    eGLSL
  };
};

struct GfxDescriptor
{
  using handle = std::variant<GfxBuffer::handle, GfxImage2D::handle>;
};

struct GfxDescriptorSet
{
  using handle  = terra::handle<GfxDescriptorSet>;
  using rhandle = uint32_t;
  enum class DescriptorType
  {
    eStorageBuffer, // storage buffer handle
    eReadOnlyImage, // image handle
    eConstantData   // ubo buffer handle
  };
};

} // namespace terra