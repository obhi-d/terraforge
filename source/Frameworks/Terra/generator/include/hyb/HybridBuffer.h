
#pragma once

#include "Common.h"
#include "GfxDevice.h"
#include "Image.h"
#include "Sampler2D.h"

namespace terra
{

class HybridBuffer
{
public:
  using handle                      = acl::link<HybridBuffer>;
  using hasher                      = LinkHash<handle>;
  HybridBuffer(HybridBuffer const&) = delete;
  HybridBuffer(HybridBuffer&&) noexcept;
  HybridBuffer() = default;
  HybridBuffer(Source owner, uint32_t width, uint32_t height, ImageFormatEnum type = ImageFormatEnum::eFloat,
               bool isImage = true);

  HybridBuffer& operator=(HybridBuffer const&) = delete;
  HybridBuffer& operator=(HybridBuffer&&) noexcept;

  template <typename T>
  Sampler2D<T> sampler()
  {
    return Sampler2D<T>(reinterpret_cast<T*>(offload()), width, height);
  }

  std::span<ubyte_t> ensureHost();
  void               ensureDev();
  bool               offload();
  bool               upload();

  void use(uint32_t now)
  {
    useCount = now;
    flags |= fUsed;
  }

  void read(uint32_t now)
  {
    use(now);
    readCount++;
  }

  GfxBuffer::handle getBuffer() const
  {
    return buffer;
  }

  GfxImage2D::handle getImage() const
  {
    return image;
  }

  ubyte_t const* getData() const
  {
    return data.get();
  }

  uint32_t lastUsed() const
  {
    return useCount;
  }

  size_t size() const
  {
    return height * width * getBaseSize(format);
  }

  bool isDetached() const
  {
    return (readCount >= readers);
  }

  void clear();

private:
  std::unique_ptr<ubyte_t[]> data;
  enum Flags : uint16_t
  {
    fLocked = 1 << 0,
    fUsed   = 1 << 1,
    fImage  = 1 << 2
  };
  union
  {
    GfxBuffer::handle  buffer = GfxBuffer::handle{};
    GfxImage2D::handle image;
  };

  HDataSource owner;

  uint32_t        width     = 0;
  uint32_t        height    = 0;
  uint32_t        useCount  = 0;
  uint32_t        readCount = 0;
  uint32_t        readers   = 0;
  ImageFormatEnum format    = ImageFormatEnum::eFloat;
  uint16_t        flags     = 0;
};

} // namespace terra