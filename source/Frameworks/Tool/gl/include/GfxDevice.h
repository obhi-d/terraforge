#pragma once
#include "GfxDeviceObjects.h"
#include "GlGfx.h"
#include "RenderDevice.h"

namespace terra
{
class GfxDevice : public RenderDevice
{
public:
  GfxBuffer::handle  createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) final;
  void               destroy(GfxBuffer::handle) final;
  GfxImage2D::handle createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                 std::byte const* data = nullptr) final;
  void               destroy(GfxImage2D::handle) final;
  GfxSampler::handle createSampler(ImageSampling) final;
  void               destroy(GfxSampler::handle) final;
  GfxDescriptorSetLayout::handle createDescriptorSetLayout(
    std::span<GfxDescriptorSetLayout::Descriptor> descriptors) final;
  void                     destroy(GfxDescriptorSetLayout::handle) final;
  GfxDescriptorSet::handle createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout) final;
  void                     destroy(GfxDescriptorSet::handle) final;
  GfxFence::handle         createFence() final;
  void                     syncFence(GfxFence::handle) final;
  GfxProgram::handle       createProgram(ShaderOptions const& options, ShaderBuilder const& sources) final;
  void                     destroy(GfxProgram::handle) final;
  GfxMesh::handle          createMeshLayout(GfxMesh::Layout const&);
  void                     destroy(GfxMesh::handle);
  std::byte*               mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) final;
  void                     unmapBuffer(GfxBuffer::handle buffer) final;
  void                     updateImage(GfxImage2D::handle image, std::span<std::byte const> data) final;
  void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) final;
  void readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<std::byte> out) final;
  void readImage(GfxImage2D::handle image, std::span<std::byte> out) final;
  void dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                       uint32_t numGroupY) final;
  void barrier(GfxBarrierFlags flags) final;
  std::shared_ptr<ShaderBuilder> createShaderBuilder(ShaderLang) final;
  void draw(GfxMesh::handle, GfxMesh::Draw const& drawDesc, GfxDescriptorSet::handle descriptorSet);
  void bindResources(GfxDescriptorSet::handle descriptorSet);
  void applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle) final;
  void init();

private:
  gl::GLuint createShader(ShaderType, std::span<gl::GLchar const*> sources, std::span<gl::GLint> lengths);
  GfxFeature   features;
  GfxResources resources;
};
} // namespace terra