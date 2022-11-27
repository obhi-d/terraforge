#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include "RenderResource.h"

namespace terra
{

struct ImageData
{
  std::unique_ptr<ubyte_t[]> data;
  uint32_t                     width  = 0;
  uint32_t                     height = 0;
  ImageFormat                  format = ImageFormat::eFloat;
};

struct ImageCodec
{
  virtual bool loadImage(ImageData& dst, std::filesystem::path) = 0;
};
} // namespace terra