
#include "hyb/GpuDataSource.h"
#include "Terra.h"
#include "hyb/HybridPipeline.h"

namespace terra
{

bool GpuImage::isPushable() const
{
  return width != 0 && height != 0 && data;
}

void GpuImage::destroyHandle()
{
  terra::get().getDevice().destroy(handle);
  handle = {};
}

void GpuImage::upload()
{
  if (!handle && data)
  {
    handle =
      terra::get().getDevice().create2DImage(GfxStorageClass::eStaticDeviceReadonly, width, height, format, data.get());
  }
}

void GpuCurveData::destroyHandle()
{
  terra::get().getDevice().destroy(handle);
  handle = {};
}

void GpuCurveData::upload()
{
  auto& dev = terra::get().getDevice();
  if (!handle)
  {
    auto size = this->size();
    handle    = dev.createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::Usage::fStorage, size);
    if (!handle)
      return;
    ubyte_t* data = dev.mapBuffer(handle, 0, size);
    if (!data)
      return;
    auto     nbpts  = (uint32_t)spline.get_x().size();
    uint32_t offset = 0;
    float    c0     = spline.get_c0();
    std::memcpy(data + offset, &nbpts, 4);
    offset += 4;
    std::memcpy(data + offset, &c0, 4);
    offset += 4;
    std::memcpy(data + offset, spline.get_x().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_y().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_b().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_c().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_d().data(), nbpts * 4);
    dev.unmapBuffer(handle);
  }
}
} // namespace terra