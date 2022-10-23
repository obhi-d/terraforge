
#include "Image.h"
#include "ImageCodec.h"
#include "RenderDevice.h"
#include "Terra.h"

namespace terra
{

void Image::unload()
{
  auto& main = Terra::get();

  if (handle)
    main.getDevice().destroy(handle);
  handle = {};
}

void Image::remove(dshandle node)
{
  Dependency::remove(node);
  if (isDetached())
    unload();
}

bool Image::ensure(Pipeline&)
{
  if (handle)
    return true;

  auto& main  = Terra::get();
  auto  codec = main.getImageCodeFor(source.extension().u8string());
  if (!codec)
    return false;

  ImageData data;
  if (!codec->loadImage(data, source))
    return false;

  width  = data.width;
  height = data.height;
  format = data.format;

  if (!handle)
    handle = main.getDevice().createImage(GfxStorageClass::eStaticDeviceReadonly, data.width, data.height, data.format,
                                          data.data.get());
  return (bool)handle;
}

bool Image::fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  std::u8string path;
  if (!getFromDataStream(dataStream, serialIdx, path))
    return false;
  source = path;
  return true;
}

void Image::toDataStreamImpl(std::vector<uint8_t>& dataStream) const
{
  std::u8string path = this->source.u8string();
  addToDataStream(dataStream, path);
}

bool ImageSource::fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  bool  result = true;
  auto& main   = terra::get();
  result &= getFromDataStream(dataStream, serialIdx, uvScale);
  result &= getFromDataStream(dataStream, serialIdx, uvOffset);
  result &= getFromDataStream(dataStream, serialIdx, defaultValue);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintMin);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintMax);
  result &= getFromDataStream(dataStream, serialIdx, sampling.first);
  result &= getFromDataStream(dataStream, serialIdx, sampling.second);
  result &= getFromDataStream(dataStream, serialIdx, source.reserved);
  sampler = main.getSampler(sampling);
  return result;
}

void ImageSource::toDataStreamImpl(std::vector<uint8_t>& dataStream) const
{
  addToDataStream(dataStream, uvScale);
  addToDataStream(dataStream, uvOffset);
  addToDataStream(dataStream, defaultValue);
  addToDataStream(dataStream, tileConstraintMin);
  addToDataStream(dataStream, tileConstraintMax);
  addToDataStream(dataStream, sampling.first);
  addToDataStream(dataStream, sampling.second);
  addToDataStream(dataStream, source.reserved);
}

std::pair<dshandle, bool> ImageSource::setParamSourceImpl(uint32_t paramIdx, dshandle h)
{
  auto const& ds = get().get<DataSource>(h);
  if (ds.getFormat().type != DataType::eImage)
    return std::pair<dshandle, bool>(source, false);
  std::swap(source.reserved, h.reserved);
  return std::pair<dshandle, bool>(h, true);
}

bool ImageSource::ensure(Pipeline& p) 
{
  if (DataSource::isValid(source))
  {
    return get().get<DataSource>(source).ensure(p);
  }
  return true;
}

} // namespace terra