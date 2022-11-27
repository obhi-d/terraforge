
#include "ImageCodec.h"

namespace terra
{

struct ImageSerializer : public ImageCodec
{
  void      saveImage(ImageData const&, std::filesystem::path);
  bool      loadImage(ImageData&, std::filesystem::path) final;
  void      loadImageRgba(std::span<ubyte_t*> rows, uint32_t width, uint32_t height, std::filesystem::path);
  void      loadImageGray(std::span<ubyte_t*> rows, uint32_t width, uint32_t height, std::filesystem::path);
};

}