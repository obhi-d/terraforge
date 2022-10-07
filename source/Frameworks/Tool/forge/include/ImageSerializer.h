
#include "ImageCodec.h"

namespace terra
{

struct ImageSerializer : public ImageCodec
{
  void      saveImage(ImageData const&, std::filesystem::path);
  ImageData loadImage(std::filesystem::path) final;
  void      loadImageRgba(std::span<std::byte*> rows, uint32_t width, uint32_t height, std::filesystem::path);
};

}