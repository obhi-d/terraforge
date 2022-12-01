#pragma once

#include "RenderResource.h"
#include <vector>

namespace terra
{
struct GfxDevice;
class Terra;

class GpuBuffer
{
public:
  GpuBuffer(GfxDevice& dev) : cd(dev) {}
  ~GpuBuffer();

  void setDesc(GfxBuffer::Usage usage, GfxStorageClass storage)
  {
    this->usage   = usage;
    this->storage = storage;
  }

  auto get() const
  {
    return handle;
  }

  auto getSize() const
  {
    return size;
  }

  void setSize(uint32_t size)
  {
    size = std::max(size, this->size);
    if (size != this->size && handle)
    {
      this->size = size;
      if (handle)
        pendingDeletion = handle;
      handle = {};
    }
  }

  void     ensure();
  ubyte_t* map(uint32_t offset, uint32_t size);
  void     unmap();

private:
  GfxDevice&        cd;
  GfxBuffer::Usage  usage;
  GfxStorageClass   storage = GfxStorageClass::eStaticDeviceReadonly;
  GfxBuffer::handle handle;
  uint32_t          size = 0;

  GfxBuffer::handle pendingDeletion;
};
} // namespace terra
