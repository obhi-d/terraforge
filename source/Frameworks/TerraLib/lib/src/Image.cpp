
#include "Image.h"
#include "ImageCodec.h"
#include "ComputeDevice.h"
#include "Terra.h"

namespace terra
{

void Image::unload()
{
  auto& main = Terra::get();
  data = {};
  updateVersion();
}

/*
void Image::remove(dshandle node)
{
  Dependency::remove(node);
  if (isDetached())
    unload();
}
*/

bool Image::load()
{
  if (this->data)
    return true;
  auto& main  = Terra::get();
  auto  ext   = source.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c)
                 {
                   return std::tolower(c);
                 });
  auto  codec = main.getImageCodeFor(ext);
  if (!codec)
    return false;

  ImageData data;
  if (!codec->loadImage(data, source))
    return false;

  this->width  = data.width;
  this->height = data.height;
  this->format = data.format;
  this->data   = std::move(data.data);
  if (this->data != nullptr)
  {
    updateVersion();
    return true;
  }
  return false;
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

HelpInfo Image::getHelpInfo(HelpType type, int param) const
{
  static HelpInfo output = {.help = "@imageOut.help"_ls, .tooltip = "@imageOut.tip"_ls};
  static HelpInfo main   = {.help = "@image.help"_ls, .tooltip = "@image.tip"_ls};
  switch (type)
  {
  case HelpType::eDataSource:
    return main;
  case HelpType::eOutput:
    return output;
  }
  return {};
}

/*
bool ImageSource::fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  bool  result = true;
  auto& main   = terra::get();
  result &= getFromDataStream(dataStream, serialIdx, uvScale);
  result &= getFromDataStream(dataStream, serialIdx, uvOffset);
  result &= getFromDataStream(dataStream, serialIdx, defaultValue);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintOffset);
  result &= getFromDataStream(dataStream, serialIdx, tileConstraintSize);
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
  addToDataStream(dataStream, tileConstraintOffset);
  addToDataStream(dataStream, tileConstraintSize);
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
*/
} // namespace terra