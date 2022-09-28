
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace terra
{
using uint32 = std::uint32_t;
using uint   = uint32;
using int32  = std::int32_t;
using float2 = std::array<float, 2>;
using float4 = std::array<float, 4>;
using int4   = std::array<int, 4>;
using int2   = std::array<int, 4>;

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
} // namespace terra