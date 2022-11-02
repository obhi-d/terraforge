#pragma once

namespace terra
{

template <typename T>
struct Property
{
  Property() = default;
  template <typename...Args>
  Property(Args&&...v) noexcept : value(std::forward<Args>(v)...)
  {}

  inline T& get() noexcept
  {
    return value;
  }

  inline T const& get() const noexcept 
  {
    return value;
  }

  inline T const* operator->() const noexcept
  {
    return &value;
  }

  inline T* operator->() noexcept
  {
    return &value;
  }

  inline operator T&() noexcept
  {
    return value;
  }

  inline operator T const&() const noexcept
  {
    return value;
  }

  inline Property& operator=(T ivalue) noexcept
  {
    value = ivalue;
    return *this;
  }

  inline auto operator<=>(Property const& other) const noexcept = default;

  inline auto operator<=>(T const& other) const noexcept 
  {
    return value <=> other.value;
  }

  T value = {};
};

} // namespace terra