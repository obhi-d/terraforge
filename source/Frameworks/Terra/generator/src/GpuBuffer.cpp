
#include "GpuBuffer.h"
#include "Terra.h"

namespace terra
{

GpuBuffer::~GpuBuffer()
{
  auto& cd = terra::get().getDevice();
  if (pendingDeletion)
    cd.destroy(pendingDeletion);
  if (handle)
    cd.destroy(handle);
}

void GpuBuffer::ensure()
{
  auto& cd = terra::get().getDevice();
  if (pendingDeletion)
    cd.destroy(pendingDeletion);
  pendingDeletion = {};
  if (!handle)
    handle = cd.createBuffer(storage, usage, size);
}

ubyte_t* GpuBuffer::map(uint32_t offset, uint32_t size)
{
  auto& cd = terra::get().getDevice();
  return cd.mapBuffer(handle, offset, size);
}

void GpuBuffer::unmap()
{
  auto& cd = terra::get().getDevice();
  cd.unmapBuffer(handle);
}

} // namespace terra