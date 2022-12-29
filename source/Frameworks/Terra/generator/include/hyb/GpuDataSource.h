
#include "CurveData.h"
#include "Image.h"
#include "hyb/HybridBuffer.h"
#include "hyb/ShaderProgramInstance.h"

namespace terra
{

class HybridPipeline;
class GpuImage : public Image
{
public:
  ~GpuImage()
  {
    destroyHandle();
  }

  GfxImage::handle getHandle(uint32_t& hversion)
  {
    if (hversion != version)
      destroyHandle();
    upload();
    hversion = version;
    return handle;
  }

  GfxImage::handle getHandle() const final
  {
    return handle;
  }

  void destroyHandle();
  bool isPushable() const final;

  void upload();

  GfxImage::handle handle;
};

class GpuCurveData : public CurveData
{
public:
  void destroyHandle();
  void upload();

  GfxBuffer::handle& getHandle(uint32_t& hversion)
  {
    if (hversion != version)
      destroyHandle();
    upload();
    hversion = version;
    return handle;
  }

  uint32_t size() const
  {
    return (1 + 1 + 5 * (uint32_t)spline.get_x().size()) * 4;
  }

  GfxBuffer::handle handle;
};

} // namespace terra