
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
  HybridBuffer(Source owner_, uint32_t width_, uint32_t height_, ImageFormatEnum type = ImageFormatEnum::eFloat,
               bool isImage = true);

  HybridBuffer& operator=(HybridBuffer const&) = delete;
  HybridBuffer& operator=(HybridBuffer&&) noexcept;

  template <typename T>
  Sampler2D<T> sampler()
  {
    return Sampler2D<T>(reinterpret_cast<T*>(offload()), width_, height_);
  }

  std::span<ubyte_t> ensureHost();
  void               ensureDev();
  bool               offload();
  bool               upload();

  void use(uint32_t now)
  {
    useCount_ = now;
    flags_ |= fUsed;
  }

  void read(uint32_t now)
  {
    use(now);
    readCount_++;
  }

  GfxBuffer::handle buffer() const
  {
    return buffer_;
  }

  GfxImage::handle image() const
  {
    return image_;
  }

  ubyte_t const* data() const
  {
    return data_.get();
  }

  uint32_t lastUsed() const
  {
    return useCount_;
  }

  size_t size() const
  {
    return height_ * width_ * getBaseSize(format_);
  }

  bool isDetached() const
  {
    return (readCount_ >= readers_);
  }

  uint32_t width() const
  {
    return width_;
  }

  uint32_t height() const
  {
    return height_;
  }

  void clear();
  void upload(std::span<ubyte_t const>);

  HDataSource owner() const
  {
    return owner_;
  }

private:
  std::unique_ptr<ubyte_t[]> data_;
  enum Flags : uint16_t
  {
    fLocked = 1 << 0,
    fUsed   = 1 << 1,
    fImage  = 1 << 2
  };
  union
  {
    GfxBuffer::handle buffer_ = GfxBuffer::handle{};
    GfxImage::handle  image_;
  };

  HDataSource owner_;

  uint32_t        width_     = 0;
  uint32_t        height_    = 0;
  uint32_t        useCount_  = 0;
  uint32_t        readCount_ = 0;
  uint32_t        readers_   = 0;
  ImageFormatEnum format_    = ImageFormatEnum::eFloat;
  uint16_t        flags_     = 0;
};

} // namespace terra