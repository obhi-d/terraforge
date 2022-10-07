
#include "ImageCodec.h"
#include "ImageData.h"
#include "RenderDevice.h"
#include "Terra.h"

namespace terra
{
bool ImageData::reload()
{
  auto& main  = Terra::get();
  auto  codec = main.getImageCodeFor(source.extension().u8string());
  if (!codec)
  {
    return false;
  }
  auto data = codec->loadImage(source);
  if (!data.data)
    return false;
  *this = std::move(data);
  if (handle)
    main.getDevice().destroy(handle);
  ensure();
  if (!handle)
    return false;
  propagate(NodeEvent::eValueModified);
  return true;
}

void ImageData::unload()
{
  auto& main = Terra::get();

  if (handle)
    main.getDevice().destroy(handle);
  handle = {};
}

void ImageData::remove(hnode node)
{
  Dependency::remove(node);
  if (isDetached())
    unload();
}

void ImageData::ensure()
{
  auto& main = Terra::get();

  if (!handle)
    handle = main.getDevice().createImage(GfxStorageClass::eStaticDeviceReadonly, width, height, format, data.get());
}

bool ImageData::fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  std::u8string path;
  if (!getFromDataStream(dataStream, serialIdx, path))
    return false;
  source = path;
  return reload();
}

void ImageData::toDataStream(std::vector<uint8_t>& dataStream) const
{
  std::u8string path = this->source.u8string();
  addToDataStream(dataStream, path);
}

bool ImageSource::fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  bool  result = true;
  auto& main = terra::get();
  result       &= getFromDataStream(dataStream, serialIdx, defaultValue);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintMin);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintMax);
  result &= getFromDataStream(dataStream, serialIdx, sampling.first);
  result &= getFromDataStream(dataStream, serialIdx, sampling.second);
  sampler            = main.getSampler(sampling);
  bool isValidSource = false;
  result &= getFromDataStream(dataStream, serialIdx, isValidSource);
  if (!isValidSource)
    return result;
  bool isSourceNode = false;
  result &= getFromDataStream(dataStream, serialIdx, isSourceNode);
  if (isSourceNode)
  {
    int32_t id;
    result &= getFromDataStream(dataStream, serialIdx, id);
    source = hnode(id);
  }
  else
  {
    std::u8string path;
    result &= getFromDataStream(dataStream, serialIdx, path);
    source = main.getImage(path);
  }
  return result;
}
void ImageSource::toDataStream(std::vector<uint8_t>& dataStream) const
{
  addToDataStream(dataStream, defaultValue);
  addToDataStream(dataStream, tileConstraintMin);
  addToDataStream(dataStream, tileConstraintMax);
  addToDataStream(dataStream, sampling.first);
  addToDataStream(dataStream, sampling.second);

  std::u8string path;
  auto&         main          = Terra::get();
  bool          isValidSource = source.index() != 0;
  addToDataStream(dataStream, isValidSource);
  if (!isValidSource)
    return;
  bool isSourceNode = std::holds_alternative<hnode>(source);
  addToDataStream(dataStream, isSourceNode);
  if (isSourceNode)
  {
    int32_t node = std::get<hnode>(source);
    addToDataStream(dataStream, node);
  }
  else
  {
    auto const& image = main.getImage(std::get<ImageDataIdx>(source));
    addToDataStream(dataStream, image.source.u8string());
  }
}

bool ImageSource::isValidSource() const
{
  return source.index() != 0 && ((std::holds_alternative<ImageDataIdx>(source) && std::get<ImageDataIdx>(source)) ||
                                 (std::holds_alternative<hnode>(source) && Node::isValid(std::get<hnode>(source))));
}
} // namespace terra