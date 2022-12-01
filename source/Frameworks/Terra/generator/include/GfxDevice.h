

#pragma once

#include "RenderResource.h"
#include "ShaderBuilder.h"
#include <span>

namespace terra
{
struct ShaderBuilder;
struct GfxDevice
{
  using Caps = GfxFeature;

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
  virtual GfxImage2D::handle createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                         ubyte_t const* data = nullptr, GfxImage2D::Swizzle swizzle = {},
                                         uint32 mipLevels = 1)                                        = 0;
  virtual GfxImage2D::handle createImageArray(GfxStorageClass storage, uint32_t width, uint32_t height,
                                              ImageFormat format, std::span<ubyte_t const*> data = {},
                                              GfxImage2D::Swizzle swizzle = {}, uint32 mipLevels = 1) = 0;
  virtual void               destroy(GfxImage2D::handle)                                              = 0;
  /// @brief Create a sampler
  virtual GfxSampler::handle createSampler(ImageSampling) = 0;
  virtual void               destroy(GfxSampler::handle)  = 0;
  /// @brief Should create a compute shader object
  /// @param sources compute shader sources
  /// @return compute shader handle
  virtual GfxProgram::handle createProgram(std::span<ShaderOptions> options, ShaderBuilder const& code) = 0;
  virtual GfxProgram::handle createFullscreenProgram(std::span<std::string_view> code)                  = 0;
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

  virtual GfxBindlessLayout::handle createBindlessLayout(std::span<GfxBindlessLayout::Entry const> entries) = 0;
  virtual void                      destroy(GfxBindlessLayout::handle)                                      = 0;
  // @brief Bindless descriptor gets deleted at the end of the frame
  virtual GfxBindlessDescriptor::handle pushBindlessDescriptor(GfxBindlessLayout::handle descriptorLayout,
                                                               std::span<ubyte_t const>  data) = 0;

  virtual GfxMesh::handle createMeshLayout(GfxMesh::Layout const&) = 0;

  /// @brief Map buffer for cpu upload
  /// @param buffer buffer handle
  /// @return CPU data to be used for memcpy
  virtual ubyte_t* mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) = 0;
  virtual void     unmapBuffer(GfxBuffer::handle buffer)                               = 0;

  /// @brief Update texture
  virtual void updateImage(GfxImage2D::handle image, std::span<ubyte_t const> data) = 0;
  /// @brief Update descriptor sett
  virtual void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) = 0;
  /// @brief Readback buffer
  virtual void readBuffer(GfxBuffer::handle buffer, uint32_t offset, std::span<ubyte_t> out) = 0;
  /// @brief Readback image
  virtual void readImage(GfxImage2D::handle image, std::span<ubyte_t> out) = 0;

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
  virtual std::shared_ptr<ShaderBuilder> createShaderBuilder(ShaderLang) = 0;

  virtual void bindResources(GfxDescriptorSet::handle descriptorSet)            = 0;
  virtual void destroy(GfxMesh::handle)                                         = 0;
  virtual void draw(GfxMesh::Draw const& drawDesc, GfxMaterial const& material) = 0;
  virtual void flushStates()                                                    = 0;
  virtual void clearBackbuffer(glm::vec4 color, bool depth = false)             = 0;
  virtual void setState(GfxState const&)                                        = 0;
  virtual void postProcessDraw(GfxProgram::handle program, std::span<GfxBindlessDescriptor::handle> descriptors,
                               std::span<GfxImage2D::handle> outputs)           = 0;
};
} // namespace terra