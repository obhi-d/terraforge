
#include "Setup.h"
#include "ResourceUtils.h"
#include "TerraMainApp.h"

namespace terra
{

bool TextureFile::load(ImageData& data)
{
  ImageSerializer ser;
  return ser.loadImage(data, path);
}

bool TextureFile::reload(TerraMainApp const& app)
{
  ImageSerializer ser;
  {
    ImageData data;
    if (ser.loadImage(data, path))
    {
      if (image)
        app.getDevice()->destroy(GfxImage::handle(image));
      image = (uint32_t)app.getDevice()->create2DImage(GfxStorageClass::eStaticDeviceReadonly, data.width, data.height,
                                                       data.format, data.data.get());
      return true;
    }
  }
  return false;
}
} // namespace terra