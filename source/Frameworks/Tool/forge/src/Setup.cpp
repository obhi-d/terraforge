
#include "ResourceUtils.h"
#include "ImageSerializer.h"
#include "TerraMainApp.h"
#include "Setup.h"

namespace terra
{

bool TextureFile::reload(TerraMainApp const& app) 
{
  ImageSerializer ser;
  {
    ImageData             data;
    if (ser.loadImage(data, path))
    {
      if (image)
        app.getDevice()->destroy(GfxImage2D::handle(image));
      image = app.getDevice()->createImage(GfxStorageClass::eStaticDeviceReadonly, data.width,
                                                          data.height, data.format, data.data.get());
      return true;
    }
  }
  return false;
}
}