#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include "ImageData.h"

namespace terra
{

struct ImageCodec
{
  virtual ImageData loadImage(std::filesystem::path) = 0;
};
} // namespace terra