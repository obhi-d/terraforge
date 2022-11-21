
#include "Setup.h"
#include "ImageSerializer.h"
#include "ResourceUtils.h"
#include "TerraMainApp.h"

namespace terra
{

bool TextureFile::reload(TerraMainApp const& app)
{
  ImageSerializer ser;
  {
    ImageData data;
    if (ser.loadImage(data, path))
    {
      if (image)
        app.getDevice()->destroy(GfxImage2D::handle(image));
      image = (uint32_t)app.getDevice()->createImage(GfxStorageClass::eStaticDeviceReadonly, data.width, data.height,
                                                     data.format, data.data.get());
      return true;
    }
  }
  return false;
}
} // namespace terra