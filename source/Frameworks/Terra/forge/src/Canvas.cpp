
#include "Canvas.h"
#include "Terra.h"

namespace terra
{

void Canvas::clearHandles()
{
  auto& dev = terra::get().getDevice();
  for (uint32_t i = 0; i < numImages; ++i)
  {
    dev.destroy(images[i]);
    images[i] = 0;
  }
  if (depthImage)
    dev.destroy(depthImage);
  depthImage = 0;
  if (pass)
    dev.destroy(pass);
  pass = 0;
}

void Canvas::clear()
{
  clearHandles();
  numImages = 0;
}

void Canvas::color(ImageFormatEnum format)
{
  clearHandles();
  desc[numImages++] = format;
}

void Canvas::depth(ImageFormatEnum format)
{
  clearHandles();
  depthDesc = format;
}

void Canvas::resize(glm::uvec2 isize)
{
  if (isize != size || !images[0])
  {
    clearHandles();
    for (uint32_t i = 0; i < numImages; ++i)
      images[i] =
        terra::get().getDevice().create2DImage(GfxStorageClass::eDynamicDeviceAccess, isize.x, isize.y, desc[i]);
    depthImage =
      terra::get().getDevice().create2DImage(GfxStorageClass::eDynamicDeviceAccess, isize.x, isize.y, depthDesc);
    size = isize;
  }
}

void Canvas::begin(bool reverseZ)
{
  auto& dev = terra::get().getDevice();

  if (!pass)
  {
    GfxPass::Attachment outputs[8];
    GfxPass::Attachment depthOut;
    for (uint32_t i = 0; i < numImages; ++i)
    {
      outputs[i].clear    = true;
      outputs[i].colorVal = glm::vec4{0.03f};
      outputs[i].image    = images[i];
    }
    depthOut.image    = depthImage;
    depthOut.clear    = true;
    depthOut.depthVal = reverseZ ? 0.f : 1.f;
    pass              = dev.createPass(std::span<GfxPass::Attachment>(outputs, outputs + numImages), depthOut);
  }
  dev.beginPass(pass);
}

void Canvas::end()
{
  terra::get().getDevice().endPass();
}

} // namespace terra