#pragma once
#include "RenderResource.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace terra
{

struct ImageData
{
  std::unique_ptr<ubyte_t[]> data;
  uint32_t                   width  = 0;
  uint32_t                   height = 0;
  ImageFormatEnum            format = ImageFormatEnum::eFloat;
};

struct ImageCodec
{
  virtual bool loadImage(ImageData& dst, std::filesystem::path) = 0;
};
} // namespace terra