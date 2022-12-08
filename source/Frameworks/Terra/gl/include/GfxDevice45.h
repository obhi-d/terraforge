#pragma once
#include "GfxDevice43.h"
#include "GfxDeviceObjects.h"
#include "GlGfx.h"

namespace terra
{
class GfxDevice45 : public GfxDevice43
{
public:
  GfxDevice45()
  {
  }

  GfxBuffer::handle  createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) override;
  GfxImage::handle create2DImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormatEnum format,
                                 ubyte_t const* data = nullptr, GfxImage::Swizzle swizzle = {}, uint32 mipLevels = 1) override;
  GfxMesh::handle    createMeshLayout(GfxMesh::Layout const&) override;
  ubyte_t*         mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) override;
  void               unmapBuffer(GfxBuffer::handle buffer) override;
  void               updateImage(GfxImage::handle image, std::span<ubyte_t const> data) override;
  void               readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out) override;
  void               readImage(GfxImage::handle image, std::span<ubyte_t> out) override;
  void               draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& mat) override;

private:
};
} // namespace terra