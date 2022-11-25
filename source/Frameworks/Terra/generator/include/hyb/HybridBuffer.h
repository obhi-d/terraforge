
#pragma once

#include "ComputeDevice.h"
#include "Image.h"
#include "Sampler2D.h"

namespace terra
{

class HybridBuffer
{
public:
  HybridBuffer(HybridBuffer const&) = delete;
  HybridBuffer(HybridBuffer&&) noexcept;
  HybridBuffer() = default;
  HybridBuffer(uint32_t width, uint32_t height, ImageFormat type = ImageFormat::eFloat, bool isImage = true);

  HybridBuffer& operator=(HybridBuffer& const) = delete;
  HybridBuffer& operator=(HybridBuffer&&) noexcept;

  template <typename T>
  Sampler2D<T> sampler()
  {
    return Sampler2D<T>(reinterpret_cast<T*>(offload()), width, height);
  }

  void       ensure();
  std::byte* offload();
  void       upload();
  void       sync();

  // lock for writing
  void lockHost()
  {
    hostVersion++;
  }

  void lockDevice()
  {
    devVersion++;
    locked = true;
  }

  void unlock()
  {
    locked = false;
  }

  bool isLocked() const
  {
    return locked;
  }

  void use(uint16_t now)
  {
    useCount = use;
  }

private:
  std::unique_ptr<std::byte[]> data;
  union
  {
    GfxBuffer::handle  buffer;
    GfxImage2D::handle image;
  };

  uint32_t    width       = 0;
  uint32_t    height      = 0;
  ImageFormat format      = ImageFormat::eFloat;
  uint16_t    hostVersion = 0;
  uint16_t    devVersion  = 0;
  uint16_t    useCount    = 0;
  bool        locked      = false;
  bool        isImage     = false;
};

} // namespace terra