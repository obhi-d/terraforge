
#include "hyb/HybridBuffer.h"
#include "Terra.h"

namespace terra
{

HybridBuffer::HybridBuffer(HybridBuffer&& hb) noexcept
{
  *this = std::move(hb);
}

HybridBuffer::HybridBuffer(Source iowner, uint32_t iwidth, uint32_t iheight, ImageFormatEnum itype, bool iisImage)
    : owner(iowner.source), width(iwidth), height(iheight), format(itype), flags(iisImage ? fImage : 0)
{
  if (DataSource::isValid(iowner.source))
    readers = get().get<DataSource>(iowner.source).countDependents(iowner.secondary);
}

HybridBuffer& HybridBuffer::operator=(HybridBuffer&& other) noexcept
{
  if (buffer)
  {
    if (flags & fImage)
      get().getDevice().destroy(image);
    else
      get().getDevice().destroy(buffer);
  }

  data      = std::move(other.data);
  buffer    = other.buffer;
  owner     = other.owner;
  width     = other.width;
  height    = other.height;
  format    = other.format;
  useCount  = other.useCount;
  readCount = other.readCount;
  readers   = other.readers;
  flags     = other.flags;

  other.buffer = {};
  return *this;
}

void HybridBuffer::ensureDev()
{
  if (buffer)
    return;
  if (flags & fImage)
  {
    image = get().getDevice().create2DImage(GfxStorageClass::eDeviceAccess, width, height, format);
  }
  else
  {
    buffer = get().getDevice().createBuffer(GfxStorageClass::eDeviceAccess, GfxBuffer::fStorage,
                                            width * height * getBaseSize(format));
  }
}

std::span<ubyte_t> HybridBuffer::ensureHost()
{
  size_t dataSize = size();
  if (!data)
    data.reset(new ubyte_t[dataSize]);
  return std::span<ubyte_t>(data.get(), dataSize);
}

bool HybridBuffer::offload()
{
  size_t size = width * height * getBaseSize(format);
  if (!size || !buffer)
    return false;
  if (!data)
  {
    data.reset(new ubyte_t[size]);
    auto& dev = get().getDevice();
    if (flags & fImage)
    {
      dev.readImage(image, std::span<ubyte_t>(data.get(), size));
      dev.destroy(image);
    }
    else
    {
      dev.readBuffer(buffer, 0, std::span<ubyte_t>(data.get(), size));
      dev.destroy(buffer);
    }
    buffer = {};
    return true;
  }
  return false;
}

bool HybridBuffer::upload()
{
  size_t dataSize = size();
  if (!dataSize || !data)
    return false;
  if (!buffer)
  {
    ensureDev();
    if (!buffer)
      return false;
    auto& dev = get().getDevice();
    if (flags & fImage)
    {
      dev.updateImage(image, std::span<ubyte_t>(data.get(), dataSize));
    }
    else
    {
      ubyte_t* bdata = dev.mapBuffer(buffer, 0, (uint32_t)dataSize);
      std::memcpy(bdata, data.get(), dataSize);
      dev.unmapBuffer(buffer);
    }
    data = {};
    return true;
  }
  return false;
}

void HybridBuffer::clear()
{
  auto& dev = get().getDevice();
  if (flags & fImage)
  {
    dev.destroy(image);
  }
  else
  {
    dev.destroy(buffer);
  }
  data = {};
}

} // namespace terra