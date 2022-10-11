#pragma once

#include "Dependency.h"
#include "RenderResource.h"
#include "Serializer.h"
#include <filesystem>
#include <memory>

namespace terra
{
class Terra;

struct ImageData : public Dependency
{
  std::filesystem::path        source;
  std::unique_ptr<std::byte[]> data;
  uint32_t                     width  = 0;
  uint32_t                     height = 0;
  ImageFormat                  format = ImageFormat::eFloat;

  GfxImage2D::handle handle;

  ImageData() = default;
  ImageData(std::filesystem::path path) : source(std::move(path)) {}
  ImageData(ImageData const&)            = default;
  ImageData(ImageData&&)                 = default;
  ImageData& operator=(ImageData const&) = default;
  ImageData& operator=(ImageData&&)      = default;

  bool reload();
  void unload();
  void ensure();

  void remove(hnode node) override;
  bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx);
  void toDataStream(std::vector<uint8_t>& dataStream) const;
};
using ImageDataPtr = std::shared_ptr<ImageData>;
using ImageDataIdx = index<ImageData>;

class Node;
struct ImageSource
{

  float              defaultValue      = 0.0f; // outside tile consraint or when image is not present
  float              uvScale           = 1.0f;
  ivec2               tileConstraintMin = {0, 0};
  ivec2               tileConstraintMax = {0, 0};
  ImageSampling      sampling;
  GfxSampler::handle sampler;

  // Derived
  std::variant<std::monostate, ImageDataIdx, hnode> source;

  ImageSource() = default;
  ImageSource(ImageDataIdx idx) : source(idx) {}
  ImageSource(hnode idx) : source(idx) {}

  bool isValidSource() const;
  bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx);
  void toDataStream(std::vector<uint8_t>& dataStream) const;
};
} // namespace terra