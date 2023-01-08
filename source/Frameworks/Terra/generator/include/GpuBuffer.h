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
  GpuBuffer() = default;
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
    if (size != this->size)
    {
      this->size      = size;
      pendingDeletion = handle;
      handle          = {};
    }
  }

  GfxBuffer::handle buffer() const
  {
    return handle;
  }

  void     ensure();
  ubyte_t* map(uint32_t offset, uint32_t size);
  void     unmap();

private:
  GfxBuffer::Usage  usage;
  GfxStorageClass   storage = GfxStorageClass::eStaticDeviceReadonly;
  GfxBuffer::handle handle;
  uint32_t          size = 0;

  GfxBuffer::handle pendingDeletion;
};

using BufferRef = std::shared_ptr<GpuBuffer>;
} // namespace terra
