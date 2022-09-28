
#pragma once
#include "ImageCodec.h"

namespace terra
{
struct RenderDevice;
class Terra
{
public:
  void addImageCodec(std::string ext, std::shared_ptr<ImageCodec> codec)
  {
    imageCodecs[ext] = codec;
  }

private:
  std::shared_ptr<RenderDevice>                                device;
  std::unordered_map<std::string, std::shared_ptr<ImageCodec>> imageCodecs;
};
} // namespace terra