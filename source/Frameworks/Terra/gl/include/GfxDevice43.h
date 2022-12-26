#pragma once
#include "GfxDevice.h"
#include "GfxDeviceObjects.h"
#include "GlGfx.h"
#include "SourceBuilder.h"

namespace terra
{
class GfxDevice43 : public GfxDevice
{
public:
  GfxDevice43()
  {
    init();
  }

  void beginFrame() override;
  void endFrame() override;

  void clearBackbuffer(glm::vec4 color, DepthClear depth) override;
  void setState(GfxState const&) override;

  GfxBuffer::handle createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) override;
  void              destroy(GfxBuffer::handle) override;
  GfxImage::handle  create2DImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormatEnum format,
                                  ubyte_t const* data = nullptr, GfxImage::Swizzle swizzle = {},
                                  uint32 mipLevels = 1) override;
  GfxImage::handle  create1DImageArray(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormatEnum format,
                                       ubyte_t const* data = {}, GfxImage::Swizzle swizzle = {},
                                       uint32 mipLevels = 1) override;
  void              destroy(GfxImage::handle) override;
  GfxSampler::handle             createSampler(ImageSampling) override;
  void                           destroy(GfxSampler::handle) override;
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
  void                     updateImage(GfxImage::handle image, std::span<ubyte_t const> data) override;
  void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) override;
  void readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out) override;
  void readImage(GfxImage::handle image, std::span<ubyte_t> out) override;
  void dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                       uint32_t numGroupY) override;
  void barrier(GfxBarrierFlags flags) override;
  std::shared_ptr<ShaderBuilder> createShaderBuilder(ShaderLang) override;
  std::shared_ptr<SourceBuilder> createSourceBuilder(ShaderLang, SourceType) override;
  void                    applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle) override;
  virtual void            bindResources(GfxDescriptorSet::handle descriptorSet);
  virtual GfxMesh::handle createMeshLayout(GfxMesh::Layout const&);
  virtual void            destroy(GfxMesh::handle);
  void                    draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& material) override;
  void                    draw(GfxMesh::Draw const& drawDesc, GfxMaterial2 const& material, Blob const& data) override;
  void                    flushStates() override;

  GfxParamLayout::handle createLayout(std::span<GfxParamLayout::Entry const> entries) override;
  void                   destroy(GfxParamLayout::handle) override;
  void postProcessDraw(GfxProgram::handle program, GfxParamLayout::handle descriptorLayout, Blob const& data) override;
  GfxProgram::handle createProgram(std::span<std::string_view> code, uint32_t activeStages) override;
  GfxProgram::handle createFullscreenProgram(std::span<std::string_view> code) override;

  Caps getCaps() const
  {
    return features;
  }

  void            blit(GfxImage::handle src, GfxImage::handle dst, Rect const& srcZone, Rect const& dstZone) override;
  GfxPass::handle createPass(std::span<GfxPass::Attachment>, GfxPass::Attachment depth = {}) override;
  void            destroy(GfxPass::handle) override;
  void            beginPass(GfxPass::handle) override;
  void            endPass() override;

protected:
  void                     setTextureParameters(gl::GLenum target, GfxImage::Swizzle);
  void                     releaseTexture(GfxImage::handle);
  void                     apply(GfxMaterial const&);
  void                     apply(GfxParamLayout::handle descriptorLayout, Blob const& data);
  void                     bindSampledTexture(gl::GLenum target, uint32_t index, SampledTexture const&);
  void                     bindTextureBuffer(uint32_t index, TextureBuffer const&);
  virtual void             draw(GfxMesh::Draw const& drawDesc);
  void                     makeResident(BindlessHandleGl::handle, GfxAccess access);
  void                     makeResident(BindlessHandleGl::handle);
  void                     destroy(BindlessHandleGl::handle);
  BindlessHandleGl::handle makeBindless(GfxImageGl const&, GfxSamplerGl const&);
  BindlessHandleGl::handle makeBindless(GfxBufferGl const&);
  BindlessHandleGl::handle makeBindless(GfxImageGl const&, StorageImage const&);

  void       init();
  gl::GLuint createShader(ShaderType, std::span<gl::GLchar const*> sources, std::span<gl::GLint> lengths);

  struct UBO
  {
    // UBO will be thrown away at end frame
    gl::GLuint buffer    = 0;
    uint32_t   capacity  = 0;
    uint32_t   available = 0;
  };

  enum Flags
  {
    HasFramebuffer = 1 << 0,
  };

  std::vector<UBO> ubo;
  gl::GLuint       fullscreenVS = 0;
  GlGfxState       state;
  GfxFeature       features;
  GfxResources     resources;
  uint32_t         frame = 0;
  uint32_t         flags = 0;
};
} // namespace terra