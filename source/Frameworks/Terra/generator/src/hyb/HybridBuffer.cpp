
#include "hyb/HybridBuffer.h"
#include "Terra.h"

namespace terra
{

HybridBuffer::HybridBuffer(HybridBuffer&& hb) noexcept
{
  *this = std::move(hb);
}

HybridBuffer::HybridBuffer(uint32_t iwidth, uint32_t iheight, ImageFormat itype, bool iisImage)
    : width(iwidth), height(iheight), format(itype), isImage(iisImage)
{}

HybridBuffer& HybridBuffer::operator=(HybridBuffer&& other) noexcept
{
  if (buffer)
  {
    if (isImage)
      get().getDevice().destroy(image);
    else
      get().getDevice().destroy(buffer);
  }

  data        = std::move(other.data);
  buffer      = other.buffer;
  width       = other.width;
  height      = other.height;
  format      = other.format;
  hostVersion = other.hostVersion;
  devVersion  = other.devVersion;
  useCount    = other.useCount;
  locked      = other.locked;
  isImage     = other.isImage;

  other.buffer = {};
  return *this;
}

void HybridBuffer::ensure()
{
  if (isImage)
  {
    get().getDevice().createImage()
  }
}

} // namespace terra