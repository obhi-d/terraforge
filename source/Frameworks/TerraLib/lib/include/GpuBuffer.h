#pragma once

#include "RenderResource.h"
#include <vector>

namespace terra
{
class Terra;
struct RenderDevice;
class GpuBuffer
{
public:
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

  void ensure();
  std::byte* map(uint32_t offset, uint32_t size);
  void       unmap();

private:
  GfxBuffer::Usage  usage;
  GfxStorageClass   storage = GfxStorageClass::eStaticGPUReadOnly;
  GfxBuffer::handle handle;
  uint32_t          size = 0;

  GfxBuffer::handle pendingDeletion;
};
} // namespace terra
