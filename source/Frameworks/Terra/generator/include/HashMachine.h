
#pragma once
#include "wyhash.h"

namespace terra
{
struct HashMachine
{
  uint64_t secret[4];
  uint64_t value = 0xaaaaaaaa;

  inline HashMachine(uint64_t initial) noexcept : value(initial)
  {
    ::make_secret(value, secret);
  }

  inline auto operator()() const noexcept
  {
    return value;
  }

  inline auto operator()(void const* key, size_t len) noexcept
  {
    return (value = ::wyhash(key, len, value, secret));
  }

  template <typename T>
  inline auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  inline auto operator<=>(HashMachine const& other) const noexcept
  {
    return value <=> other.value;
  }
};

} // namespace terra