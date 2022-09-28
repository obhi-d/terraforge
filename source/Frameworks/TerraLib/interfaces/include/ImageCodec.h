#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace terra
{

enum class ImageFormat
{
  eFloat,
  eUnorm16
};

struct ImageData
{
  std::unique_ptr<std::byte[]> data;
  uint32_t                     width;
  uint32_t                     height;
  ImageFormat                  format;
};

struct ImageCodec
{
  virtual ImageData loadImage(std::filesystem::path) = 0;
};
} // namespace terra