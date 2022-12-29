
#include "hyb/HybridBuffer.h"
#include "Terra.h"

namespace terra
{

HybridBuffer::HybridBuffer(HybridBuffer&& hb) noexcept
{
  *this = std::move(hb);
}

HybridBuffer::HybridBuffer(Source iowner, uint32_t iwidth, uint32_t iheight, ImageFormatEnum itype, bool iisImage)
    : owner_(iowner.source), width_(iwidth), height_(iheight), format_(itype), flags_(iisImage ? fImage : 0)
{
  if (DataSource::isValid(iowner.source))
    readers_ = get().get<DataSource>(iowner.source).countDependents(iowner.secondary);
}

HybridBuffer& HybridBuffer::operator=(HybridBuffer&& other) noexcept
{
  if (buffer_)
  {
    if (flags_ & fImage)
      get().getDevice().destroy(image_);
    else
      get().getDevice().destroy(buffer_);
  }

  data_      = std::move(other.data_);
  buffer_    = other.buffer_;
  owner_     = other.owner_;
  width_     = other.width_;
  height_    = other.height_;
  format_    = other.format_;
  useCount_  = other.useCount_;
  readCount_ = other.readCount_;
  readers_   = other.readers_;
  flags_     = other.flags_;

  other.buffer_ = {};
  return *this;
}

void HybridBuffer::ensureDev()
{
  if (buffer_)
    return;
  if (flags_ & fImage)
  {
    image_ = get().getDevice().create2DImage(GfxStorageClass::eDeviceAccess, width_, height_, format_);
  }
  else
  {
    buffer_ = get().getDevice().createBuffer(GfxStorageClass::eDeviceAccess, GfxBuffer::fStorage,
                                             width_ * height_ * getBaseSize(format_));
  }
}

std::span<ubyte_t> HybridBuffer::ensureHost()
{
  size_t dataSize = size();
  if (!data_)
    data_.reset(new ubyte_t[dataSize]);
  return std::span<ubyte_t>(data_.get(), dataSize);
}

bool HybridBuffer::offload()
{
  size_t size = width_ * height_ * getBaseSize(format_);
  if (!size || !buffer_)
    return false;
  if (!data_)
  {
    data_.reset(new ubyte_t[size]);
    auto& dev = get().getDevice();
    if (flags_ & fImage)
    {
      dev.readImage(image_, std::span<ubyte_t>(data_.get(), size));
      dev.destroy(image_);
    }
    else
    {
      dev.readBuffer(buffer_, 0, std::span<ubyte_t>(data_.get(), size));
      dev.destroy(buffer_);
    }
    buffer_ = {};
    return true;
  }
  return false;
}

bool HybridBuffer::upload()
{
  size_t dataSize = size();
  if (!dataSize || !data_)
    return false;
  if (!buffer_)
  {
    ensureDev();
    if (!buffer_)
      return false;
    auto& dev = get().getDevice();
    if (flags_ & fImage)
    {
      dev.updateImage(image_, std::span<ubyte_t>(data_.get(), dataSize));
    }
    else
    {
      ubyte_t* bdata = dev.mapBuffer(buffer_, 0, (uint32_t)dataSize);
      std::memcpy(bdata, data_.get(), dataSize);
      dev.unmapBuffer(buffer_);
    }
    data_ = {};
    return true;
  }
  return false;
}

void HybridBuffer::clear()
{
  auto& dev = get().getDevice();
  if (flags_ & fImage)
  {
    dev.destroy(image_);
    image_ = {};
  }
  else
  {
    dev.destroy(buffer_);
    buffer_ = {};
  }
  data_ = {};
}

void HybridBuffer::upload(std::span<ubyte_t const> data)
{
  if (!(flags_ & fImage))
    return;
  ensureDev();
  auto& dev = get().getDevice();
  dev.updateImage(image_, data);
}

} // namespace terra