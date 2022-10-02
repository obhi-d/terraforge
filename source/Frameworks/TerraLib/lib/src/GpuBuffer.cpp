
#include "Terra.h"
#include "GpuBuffer.h"

namespace terra
{

GpuBuffer::~GpuBuffer() 
{
  if (pendingDeletion)
    terra::get().getDevice().destroy(pendingDeletion);
  if (handle)
    terra::get().getDevice().destroy(handle);
}

void GpuBuffer::ensure()
{
  if (pendingDeletion)
    terra::get().getDevice().destroy(pendingDeletion);
  pendingDeletion = {};
  if (!handle)
    handle = terra::get().getDevice().createBuffer(storage, usage, size);
}

std::byte* GpuBuffer::map(uint32_t offset, uint32_t size) 
{
  return terra::get().getDevice().mapBuffer(handle, offset, size);
}

void GpuBuffer::unmap()
{
  terra::get().getDevice().unmapBuffer(handle);
}

}