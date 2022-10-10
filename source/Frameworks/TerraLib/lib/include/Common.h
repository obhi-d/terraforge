#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <cctype>

namespace terra
{
using uint32 = std::uint32_t;
using uint   = uint32;
using int32  = std::int32_t;
using vec2 = std::array<float, 2>;
using vec4 = std::array<float, 4>;
using int4   = std::array<int, 4>;
using ivec2   = std::array<int, 2>;
using uint2  = std::array<uint32, 2>;

struct Content
{
  std::unique_ptr<std::byte[]> data;
  size_t                       size = 0;

  Content()                              = default;
  Content(Content const&)                = default;
  Content(Content&&) noexcept            = default;
  Content& operator=(Content const&)     = default;
  Content& operator=(Content&&) noexcept = default;
};

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
struct handle
{
  constexpr handle() noexcept = default;
  constexpr handle(uint32_t v) noexcept : reserved(v) {}

  constexpr operator uint32_t() const noexcept
  {
    return reserved;
  }

  constexpr operator bool() const noexcept
  {
    return reserved != std::numeric_limits<uint32_t>::max();
  }

  constexpr auto operator<=>(handle const&) const noexcept = default;

  constexpr uint32_t value() const
  {
    return 0x00ffffff & reserved;
  }

public:
  uint32_t reserved = std::numeric_limits<uint32_t>::max();
};

template <typename T>
struct index
{
  constexpr index() noexcept = default;
  constexpr index(int32_t v) noexcept : reserved(v) {}

  constexpr operator int32_t() const noexcept
  {
    return reserved;
  }

  constexpr operator bool() const noexcept
  {
    return reserved < 0;
  }

  constexpr auto operator<=>(index const&) const noexcept = default;

public:
  int32_t reserved = std::numeric_limits<int32_t>::min();
};

template <typename T>
using optional_ref = std::optional<std::reference_wrapper<T>>;

class Pipeline;
class Node;
using hnode = handle<Node>;

// default values recommended by http://isthe.com/chongo/tech/comp/fnv/
/// hash a single byte
constexpr uint32_t Seed = 0x811C9DC5; // 2166136261
inline uint32_t    fnv1a(unsigned char oneByte, uint32_t hash = Seed)
{
  constexpr uint32_t Prime = 0x01000193; //   16777619
  return (oneByte ^ hash) * Prime;
}
inline uint32_t fnv1a(const void* data, size_t numBytes, uint32_t hash = Seed)
{
  assert(data);
  const unsigned char* ptr = (const unsigned char*)data;
  while (numBytes--)
    hash = fnv1a(*ptr++, hash);
  return hash;
}

inline std::u8string parseU8(std::string_view from) 
{
  std::u8string out;
  auto hexchar = [](char c) -> char8_t
  {
    c = std::toupper(c);
    return (c >= 'A') ? (c - 'A' + 10) : (c - '0');
  };
  for (size_t i = 0; i != from.size();)
  {
    if (from[i] == '\\')
    {
      i++;
      if (i < from.size())
      {
        if (from[i] == 'x' && i + 2 < from.size())
        {
          auto a = hexchar(from[i + 1]);
          a      = a << 4 | hexchar(from[i + 2]);
          out.push_back(a);
          i += 3;
          continue;
        }
      }
      else
        break;
    }
    out.push_back(from[i]);
    i++;
  }
  return out;
}
} // namespace terra