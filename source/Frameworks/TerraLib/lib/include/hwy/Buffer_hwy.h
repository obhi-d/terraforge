
#pragma once
#include "Common.h"
#include <acl/sparse_vector.hpp>
#include <hwy/aligned_allocator.h>

namespace terra
{
template <typename T>
class Buffer_hwy
{
public:
  constexpr Buffer_hwy() = default;
  Buffer_hwy(uint32_t width, uint32_t height, uint32_t lanes) : lanes_(lanes), width_(width), height_(height)
  {
    size_ = pitch() * height_;
  }
  Buffer_hwy(Buffer_hwy const& other)
  {
    *this = other;
  }
  Buffer_hwy& operator=(Buffer_hwy const& other)
  {
    if (data_)
      hwy::FreeAlignedBytes(data_, &deallocate, nullptr);

    width_  = other.width_;
    height_ = other.height_;
    lanes_  = other.lanes_;
    size_   = other.size_;

    ensure();
    std::memcpy(data_, other.data_, size_ * sizeof(T));
    return *this;
  }
  Buffer_hwy(Buffer_hwy&& other) noexcept
  {
    *this = std::move(other);
  }
  ~Buffer_hwy()
  {
    if (data_)
      hwy::FreeAlignedBytes(data_, &deallocate, nullptr);
  }

  Buffer_hwy& operator=(Buffer_hwy&& other) noexcept
  {
    if (data_)
      hwy::FreeAlignedBytes(data_, &deallocate, nullptr);

    data_       = other.data_;
    width_      = other.width_;
    height_     = other.height_;
    lanes_      = other.lanes_;
    size_       = other.size_;
    other.data_ = nullptr;
    return *this;
  }

  void swap_data(Buffer_hwy& other)
  {
    std::swap(data_, other.data_);
  }

  T* data()
  {
    if (!data_)
      ensure();
    return data_;
  }

  T const* data() const
  {
    return data_;
  }

  uint32_t size() const
  {
    return size_;
  }

  uint32_t pitch() const
  {
    return ((width_ + (lanes_ - 1)) / lanes_) * lanes_;
  }

  void ensure()
  {
    data_ = (T*)hwy::AllocateAlignedBytes(size_ * sizeof(T), &allocate, nullptr);
  }

  void randomize()
  {
    for (uint32_t i = 0; i < size_; ++i)
      data_[i] = (float)std::rand() / (float)RAND_MAX;
  }

  void fill(float value)
  {
    std::fill(data(), data() + size_, value);
  }

  uint32_t height() const
  {
    return height_;
  }

  uint32_t width() const
  {
    return width_;
  }

private:
  static void* allocate(void*, size_t size)
  {
    return mi_malloc(size);
  }

  static void deallocate(void*, void* memory)
  {
    mi_free(memory);
  }

  T*       data_   = nullptr;
  uint32_t size_   = 0;
  uint32_t width_  = 0;
  uint32_t height_ = 0;
  uint32_t lanes_  = 0;
};

struct traits
{
  using size_type                              = std::uint32_t;
  static constexpr std::uint32_t pool_size     = 8;
  static constexpr std::uint32_t idx_pool_size = 8;
  static constexpr bool          assume_pod_v  = false;
  // null
  // static constexpr T null_v = {};
  // using offset
  // using offset = acl::offset<&selfref::self>;
};

using hwybuffer      = Buffer_hwy<float>;
using hwybuffer_list = acl::sparse_vector<hwybuffer, acl::default_allocator<>, traits>;

template <typename T>
struct VBuffer_hwy
{
  Buffer_hwy<T> x;
  Buffer_hwy<T> y;

  auto pitch() const
  {
    return x.pitch();
  }

  VBuffer_hwy(VBuffer_hwy&&) noexcept            = default;
  VBuffer_hwy& operator=(VBuffer_hwy&&) noexcept = default;

  VBuffer_hwy(VBuffer_hwy const& other) noexcept : x(other.x), y(other.y) {}
  VBuffer_hwy& operator=(VBuffer_hwy const& other) noexcept
  {
    x = other.x;
    y = other.y;
    return *this;
  }

  constexpr VBuffer_hwy() = default;
  VBuffer_hwy(uint32_t width, uint32_t height, uint32_t lanes) : x(width, height, lanes), y(width, height, lanes) {}
};

using hwyvb      = VBuffer_hwy<float>;
using hwyvb_list = acl::sparse_vector<hwyvb, acl::default_allocator<>, traits>;

} // namespace terra