

#pragma once

#include "RenderResource.h"
#include "ShaderBuilder.h"
#include "SourceBuilder.h"
#include <span>

namespace terra
{
struct ShaderBuilder;
struct GfxDevice
{
  using Caps = GfxFeature;

  virtual void destroy() = 0;

  virtual void beginFrame() = 0;
  virtual void endFrame()   = 0;

  virtual Caps getCaps() const = 0;
  /// @brief Create a buffer of certain size, a suballocator is expected
  /// @param storage Buffer storage on the GPU
  /// @param usage Buffer usage
  /// @param size sizze of buffer
  /// @return Buffer handle, that can be reused
  virtual GfxBuffer::handle createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) = 0;
  virtual void              destroy(GfxBuffer::handle)                                                   = 0;
  /// @brief Create a 2D image of width x height dimension, with a single mip level
  /// @param format Image format
  /// @param width image width
  /// @param height image height
  /// @return Returns the image handle
  virtual GfxImage::handle create2DImage(GfxStorageClass storage, uint32_t width, uint32_t height,
                                         ImageFormatEnum format, ubyte_t const* data = nullptr,
                                         GfxImage::Swizzle swizzle = {}, uint32 mipLevels = 1)      = 0;
  virtual GfxImage::handle create1DImageArray(GfxStorageClass storage, uint32_t width, uint32_t numLayer,
                                              ImageFormatEnum format, ubyte_t const* data = {},
                                              GfxImage::Swizzle swizzle = {}, uint32 mipLevels = 1) = 0;
  virtual void             destroy(GfxImage::handle)                                                = 0;
  /// @brief Create a sampler
  virtual GfxSampler::handle createSampler(ImageSampling) = 0;
  virtual void               destroy(GfxSampler::handle)  = 0;
  /// @brief Should create a vertex/fragment shader object
  /// @param sources vs/fs shader sources
  /// @return vs/fs handle
  virtual GfxProgram::handle createProgram(std::span<ShaderOptions> options, ShaderBuilder const& code) = 0;
  virtual GfxProgram::handle createFullscreenProgram(std::span<std::string_view> code)                  = 0;
  virtual GfxProgram::handle createProgram(std::span<std::string_view> code, uint32_t activeStages)     = 0;
  virtual void               destroy(GfxProgram::handle)                                                = 0;
  /// @brief Create a descriptor set layout
  /// @param types handle types
  /// @return DescriptorSet handle
  virtual GfxDescriptorSetLayout::handle createDescriptorSetLayout(
    std::span<GfxDescriptorSetLayout::Descriptor> descriptors)                                  = 0;
  virtual void destroy(GfxDescriptorSetLayout::handle)                                          = 0;
  virtual void applyLayoutToProgram(GfxProgram::handle program, GfxDescriptorSetLayout::handle) = 0;
  /// @brief Create descriptor set from layout
  virtual GfxDescriptorSet::handle createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout) = 0;
  virtual void                     destroy(GfxDescriptorSet::handle)                                    = 0;

  virtual GfxParamLayout::handle createLayout(std::span<GfxParamLayout::Entry const> entries) = 0;
  virtual void                   destroy(GfxParamLayout::handle)                              = 0;
  // @brief Creat a pass
  virtual GfxPass::handle createPass(std::span<GfxPass::Attachment>, GfxPass::Attachment depth = {}) = 0;
  virtual void            destroy(GfxPass::handle)                                                   = 0;

  virtual GfxMesh::handle createMeshLayout(GfxMesh::Layout const&) = 0;

  /// @brief Map buffer for cpu upload
  /// @param buffer buffer handle
  /// @return CPU data to be used for memcpy
  virtual ubyte_t* mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) = 0;
  virtual void     unmapBuffer(GfxBuffer::handle buffer)                               = 0;

  /// @brief Update texture
  virtual void updateImage(GfxImage::handle image, std::span<ubyte_t const> data) = 0;
  /// @brief Update descriptor sett
  virtual void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) = 0;
  /// @brief Readback buffer
  virtual void readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out) = 0;
  /// @brief Readback image
  virtual void readImage(GfxImage::handle image, std::span<ubyte_t> out) = 0;

  /// @brief dispatch a compute shader
  /// @param shader shader handle
  /// @param descriptorSets Descriptor set to bind
  /// @param numGroupX dispatch size x
  /// @param numGroupY dispatch size y
  virtual void dispatchCompute(GfxProgram::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                               uint32_t numGroupY) = 0;
  /// @brief Buffer barrier
  virtual void barrier(GfxBarrierFlags flags) = 0;

  /// @brief push a fence
  virtual GfxFence::handle createFence()               = 0;
  virtual void             syncFence(GfxFence::handle) = 0;

  /// @brief Create a ShaderBuilder for a specific shader
  virtual std::shared_ptr<ShaderBuilder> createShaderBuilder(ShaderLang)             = 0;
  virtual std::shared_ptr<SourceBuilder> createSourceBuilder(ShaderLang, SourceType) = 0;

  virtual void bindResources(GfxDescriptorSet::handle descriptorSet)                                      = 0;
  virtual void destroy(GfxMesh::handle)                                                                   = 0;
  virtual void draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& material)                           = 0;
  virtual void draw(GfxMesh::Draw const& drawDesc, GfxMaterial2 const& material, Blob const& data)        = 0;
  virtual void dispatchCompute(GfxMaterial2 const& material, Blob const& data, uint32_t numGroupX,
                               uint32_t numGroupY)                                                        = 0;
  virtual void flushStates()                                                                              = 0;
  virtual void clearBackbuffer(glm::vec4 color, DepthClear depth = DepthClear::eNone)                     = 0;
  virtual void setState(GfxState const&)                                                                  = 0;
  virtual void postProcessDraw(GfxProgram::handle program, GfxParamLayout::handle descriptorLayout,
                               Blob const& data)                                                          = 0;
  virtual void blit(GfxImage::handle src, GfxImage::handle dst, Rect const& srcZone, Rect const& dstZone) = 0;
  // all draw calls must be within a pass
  virtual void beginPass(GfxPass::handle) = 0;
  virtual void endPass()                  = 0;
};
} // namespace terra