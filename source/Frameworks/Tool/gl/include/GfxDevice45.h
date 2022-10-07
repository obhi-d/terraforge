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
  GfxImage2D::handle createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                 std::byte const* data = nullptr, GfxImage2D::Swizzle swizzle = {}) override;
  GfxMesh::handle    createMeshLayout(GfxMesh::Layout const&) override;
  std::byte*         mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) override;
  void               unmapBuffer(GfxBuffer::handle buffer) override;
  void               updateImage(GfxImage2D::handle image, std::span<std::byte const> data) override;
  void               readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<std::byte> out) override;
  void               readImage(GfxImage2D::handle image, std::span<std::byte> out) override;
  void               draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& mat) override;

private:
};
} // namespace terra