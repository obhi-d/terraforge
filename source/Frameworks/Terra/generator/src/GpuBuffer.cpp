
#include "Terra.h"
#include "GpuBuffer.h"

namespace terra
{

GpuBuffer::~GpuBuffer() 
{
  if (pendingDeletion)
    cd.destroy(pendingDeletion);
  if (handle)
    cd.destroy(handle);
}

void GpuBuffer::ensure()
{
  if (pendingDeletion)
    cd.destroy(pendingDeletion);
  pendingDeletion = {};
  if (!handle)
    handle = cd.createBuffer(storage, usage, size);
}

std::byte* GpuBuffer::map(uint32_t offset, uint32_t size) 
{
  return cd.mapBuffer(handle, offset, size);
}

void GpuBuffer::unmap()
{
  cd.unmapBuffer(handle);
}

}