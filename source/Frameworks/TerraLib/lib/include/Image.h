#pragma once

#include "DataSource.h"
#include "RenderResource.h"
#include "Serializer.h"
#include <filesystem>
#include <memory>

namespace terra
{
class Terra;

struct Image : public DataSource
{
  std::filesystem::path source;
  GfxImage2D::handle    handle;
  uint32_t              width  = 0;
  uint32_t              height = 0;
  ImageFormat           format = ImageFormat::eFloat;

  Image() = default;
  Image(std::filesystem::path path) : source(std::move(path)) {}
  Image(Image const&)            = default;
  Image(Image&&)                 = default;
  Image& operator=(Image const&) = default;
  Image& operator=(Image&&)      = default;

  Type getType() const final
  {
    return Type::eImage;
  }

  DataFormat getFormat() const final
  {
    return DataFormat{.type = DataType::eImage, .scalarSubType = DataType::eFloat};
  }

  inline std::pair<dshandle, bool> setParamSourceImpl(uint32_t paramIdx, dshandle) final
  {
    return std::make_pair<dshandle, bool>({}, false);
  }

  void unload();
  bool isEnabled(Pipeline const&) final;
  bool ensure(Pipeline&) final;
  void remove(dshandle node) final;
  void fillDescriptor(Pipeline const&, GfxDescriptorSet::rhandle&, std::byte*) final;
  bool fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx) final;
  void toDataStreamImpl(std::vector<uint8_t>& dataStream) const;
};

using ImagePtr = std::shared_ptr<Image>;

class Node;
struct ImageSource : public DataSource
{
  vec2               uvScale        = vec2{1.0f, 1.0f};
  vec2               uvOffset       = vec2{0.0f, 0.0f};
  ivec2              tileConstraintMax = {-1, -1}; // outside tile consraint or when image is not present
  ivec2              tileConstraintMin = {-1, -1}; // outside tile consraint or when image is not present
  float              defaultValue   = 1.0f;
  ImageSampling      sampling;
  GfxSampler::handle sampler;
  dshandle           source;

  ImageSource() = default;
  ImageSource(dshandle idx) : source(idx) {}

  inline Type getType() const final
  {
    return Type::eImageSource;
  }

  inline DataFormat getFormat() const final
  {
    return DataFormat{.type = DataType::eImageSource, .scalarSubType = DataType::eFloat};
  }

  inline bool isWithinTile(ivec2 tile) const 
  {
    return DataSource::isWithinTile(tile, tileConstraintMin, tileConstraintMax);
  }

  bool        isEnabled(Pipeline const&) final;
  bool        ensure(Pipeline&) final;
  inline void accept(dshandle source, Event) final {}

  std::pair<dshandle, bool> setParamSourceImpl(uint32_t paramIdx, dshandle) final;

  void fillDescriptor(Pipeline const&, GfxDescriptorSet::rhandle&, std::byte*) final;
  bool fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx) final;
  void toDataStreamImpl(std::vector<uint8_t>& dataStream) const final;
};

} // namespace terra