

#pragma once

#include "RenderResource.h"

namespace terra
{
struct RenderDevice
{

  /// @brief Create a buffer of certain size, a suballocator is expected
  /// @param storage Buffer storage on the GPU
  /// @param usage Buffer usage
  /// @param size sizze of buffer
  /// @return Buffer handle, that can be reused
  virtual GfxBuffer::handle createBuffer(GfxBuffer::Storage storage, GfxBuffer::Usage usage, uint32_t size) = 0;
  /// @brief Create a 2D image of width x height dimension, with a single mip level
  /// @param format Image format
  /// @param width image width
  /// @param height image height
  /// @return Returns the image handle
  virtual GfxImage2D::handle createImage(ImageData const& data) = 0;
  /// @brief Should create a compute shader object
  /// @param sources compute shader sources
  /// @return compute shader handle
  virtual GfxCompute::handle createComputeShader(std::span<std::string_view> sources, GfxCompute::Language) = 0;
  /// @brief Create a descriptor set
  /// @param types handle types
  /// @return DescriptorSet handle
  virtual GfxDescriptorSet::handle createDescriptorSet(std::span<GfxDescriptorSet::DescriptorType> types);

  /// @brief Map buffer for cpu upload
  /// @param buffer buffer handle
  /// @return CPU data to be used for memcpy
  virtual std::byte* mapBuffer(GfxBuffer::handle buffer) = 0;
  /// @brief Map should be followed by an unmap call
  /// @param buffer buffer handle
  virtual void unmapBuffer(GfxBuffer::handle buffer) = 0;
  /// @brief Update texture
  virtual void updateImage(GfxImage2D::handle image, std::span<std::byte> data);
  /// @brief Update descriptor sett
  virtual void updateDescriptorSet(GfxDescriptorSet::handle, std::span<GfxDescriptorSet::rhandle> handles);

  /// @brief dispatch a compute shader
  /// @param shader shader handle
  /// @param descriptorSets Descriptor set to bind
  /// @param numGroupX dispatch size x
  /// @param numGroupY dispatch size y
  virtual void dispatchCompute(GfxCompute::handle shader, GfxDescriptorSet::handle descriptorSet, uint32_t numGroupX,
                               uint32_t numGroupY) = 0;
  // @brief Buffer barrier
  virtual void bufferBarrier(GfxBuffer::BarrierFlags flags) = 0;
};
} // namespace terra