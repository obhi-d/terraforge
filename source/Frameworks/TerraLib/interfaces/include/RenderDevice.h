

#pragma once

#include "RenderResource.h"

namespace terra
{
struct ShaderBuilder;
struct RenderDevice
{

  /// @brief Create a buffer of certain size, a suballocator is expected
  /// @param storage Buffer storage on the GPU
  /// @param usage Buffer usage
  /// @param size sizze of buffer
  /// @return Buffer handle, that can be reused
  virtual GfxBuffer::handle createBuffer(GfxStorageClass storage, GfxBuffer::Usage usage, uint32_t size) = 0;
  virtual void              destroy(GfxBuffer::handle) = 0;
  /// @brief Create a 2D image of width x height dimension, with a single mip level
  /// @param format Image format
  /// @param width image width
  /// @param height image height
  /// @return Returns the image handle
  virtual GfxImage2D::handle createImage(GfxStorageClass storage, uint32_t width, uint32_t height, ImageFormat format,
                                         std::byte const* data = nullptr) = 0;
  virtual void               destroy(GfxImage2D::handle)                  = 0;
  /// @brief Create a sampler
  virtual GfxSampler::handle createSampler(ImageSampling) = 0;
  virtual void               destroy(GfxSampler::handle)  = 0;
  /// @brief Should create a compute shader object
  /// @param sources compute shader sources
  /// @return compute shader handle
  virtual GfxCompute::handle createComputeShader(std::span<std::string_view> sources, GfxCompute::Language) = 0;
  virtual void               destroy(GfxCompute::handle)                                                    = 0;
  /// @brief Create a descriptor set layout
  /// @param types handle types
  /// @return DescriptorSet handle
  virtual GfxDescriptorSetLayout::handle createDescriptorSetLayout(
    std::span<GfxDescriptorSetLayout::Descriptor> descriptors) = 0;
  virtual void destroy(GfxDescriptorSetLayout::handle)         = 0;
  /// @brief Create descriptor set from layout
  virtual GfxDescriptorSet::handle createDescriptorSet(GfxDescriptorSetLayout::handle descriptorLayout) = 0;
  virtual void                     destroy(GfxDescriptorSet::handle)                                    = 0;

  /// @brief Map buffer for cpu upload
  /// @param buffer buffer handle
  /// @return CPU data to be used for memcpy
  virtual std::byte* mapBuffer(GfxBuffer::handle buffer, uint32_t offset, uint32_t size) = 0;
  virtual void unmapBuffer(GfxBuffer::handle buffer) = 0;

  /// @brief Update texture
  virtual void updateImage(GfxImage2D::handle image, std::span<std::byte> data) = 0;
  /// @brief Update descriptor sett
  virtual void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles) = 0;
  /// @brief Readback buffer
  virtual std::unique_ptr<std::byte[]> readBuffer(GfxBuffer::handle buffer) = 0;
  /// @brief Readback image
  virtual std::unique_ptr<std::byte[]> readImage(GfxImage2D::handle image) = 0;

  /// @brief dispatch a compute shader
  /// @param shader shader handle
  /// @param descriptorSets Descriptor set to bind
  /// @param numGroupX dispatch size x
  /// @param numGroupY dispatch size y
  virtual void dispatchCompute(GfxCompute::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                               uint32_t numGroupY) = 0;
  /// @brief Buffer barrier
  virtual void barrier(GfxBarrierFlags flags) = 0;

  /// @brief push a fence
  virtual GfxFence::handle createFence() = 0;
  virtual void             syncFence(GfxFence::handle) = 0;

  /// @brief Create a ShaderBuilder for a specific shader
  virtual std::shared_ptr<ShaderBuilder> createShaderBuilder(GfxCompute::Language) = 0;

};
} // namespace terra