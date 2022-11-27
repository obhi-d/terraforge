#pragma once
#include "ComputeDevice.h"
#include "GfxDeviceObjects.h"
#include "GlGfx.h"

namespace terra
{
class GfxDevice43 : public ComputeDevice
{
public:
  GfxDevice43()
  {
    init();
  }
  void clearBackbuffer(glm::vec4 color, bool depth = false) override;
  void setState(GfxState const&) override;

  GfxBuffer::handle  createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) override;
  void               destroy(GfxBuffer::handle) override;
  GfxImage2D::handle createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                 ubyte_t const* data = nullptr, GfxImage2D::Swizzle swizzle = {},
                                 uint32 mipLevels = 1) override;
  GfxImage2D::handle createImageArray(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                      std::span<ubyte_t const*> data = {}, GfxImage2D::Swizzle swizzle = {},
                                      uint32 mipLevels = 1) override;
  void               destroy(GfxImage2D::handle) override;
  GfxSampler::handle createSampler(ImageSampling) override;
  void               destroy(GfxSampler::handle) override;
  GfxDescriptorSetLayout::handle createDescriptorSetLayout(
    std::span<GfxDescriptorSetLayout::Descriptor> descriptors) override;
  void                     destroy(GfxDescriptorSetLayout::handle) override;
  GfxDescriptorSet::handle createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout) override;
  void                     destroy(GfxDescriptorSet::handle) override;
  GfxFence::handle         createFence() override;
  void                     syncFence(GfxFence::handle) override;
  GfxProgram::handle       createProgram(std::span<ShaderOptions> options, ShaderBuilder const& sources) override;
  void                     destroy(GfxProgram::handle) override;
  ubyte_t*                 mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) override;
  void                     unmapBuffer(GfxBuffer::handle buffer) override;
  void                     updateImage(GfxImage2D::handle image, std::span<ubyte_t const> data) override;
  void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) override;
  void readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out) override;
  void readImage(GfxImage2D::handle image, std::span<ubyte_t> out) override;
  void dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                       uint32_t numGroupY) override;
  void barrier(GfxBarrierFlags flags) override;
  std::shared_ptr<ShaderBuilder> createShaderBuilder(ShaderLang) override;
  void                    applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle) override;
  virtual void            bindResources(GfxDescriptorSet::handle descriptorSet);
  virtual GfxMesh::handle createMeshLayout(GfxMesh::Layout const&);
  virtual void            destroy(GfxMesh::handle);
  virtual void            draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& material);
  virtual void            flushStates();
  Caps                    getCaps() const
  {
    return features;
  }

protected:
  void       init();
  gl::GLuint createShader(ShaderType, std::span<gl::GLchar const*> sources, std::span<gl::GLint> lengths);

  GlGfxState   state;
  GfxFeature   features;
  GfxResources resources;
};
} // namespace terra