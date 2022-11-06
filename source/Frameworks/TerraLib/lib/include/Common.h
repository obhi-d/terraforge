#pragma once
#include <acl/linear_arena_allocator.hpp>
#include <array>
#include <cassert>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mimalloc-2.0/mimalloc.h>
#include <optional>
#include <stdexcept>
#include <semaphore>
#include <future>
#include <vector>

#define ENUM_FLAGS(Enum)                                                                                               \
  inline Enum operator|(Enum a, Enum b)                                                                                \
  {                                                                                                                    \
    return (Enum)((uint32_t)a | (uint32_t)b);                                                                          \
  }                                                                                                                    \
  inline bool operator&(Enum a, Enum b)                                                                                \
  {                                                                                                                    \
    return ((uint32_t)a & (uint32_t)b) != 0;                                                                           \
  }                                                                                                                    \
  inline Enum& operator&=(Enum& a, Enum b)                                                                             \
  {                                                                                                                    \
    return (Enum&)((uint32_t&)a &= (uint32_t)b);                                                                       \
  }                                                                                                                    \
  inline Enum& operator|=(Enum& a, Enum b)                                                                             \
  {                                                                                                                    \
    return (Enum&)((uint32_t&)a |= (uint32_t)b);                                                                       \
  }                                                                                                                    \
  inline bool operator!(Enum a)                                                                                        \
  {                                                                                                                    \
    return ((uint32_t)a != 0);                                                                                         \
  }

namespace terra
{

namespace consts
{
static constexpr int32_t X     = 501125321;
static constexpr int32_t Y     = 1136930381;
static constexpr int32_t Z     = 1720413743;
static constexpr int32_t W     = 1066037191;
static constexpr float   root2 = 1.4142135623730950488f;
static constexpr float   root3 = 1.7320508075688772935f;
static constexpr float   pi    = 3.141592653589793238462643383279502884f;
} // namespace consts

using uint32 = std::uint32_t;
using uint   = uint32;
using int32  = std::int32_t;
using vec2   = std::array<float, 2>;
using vec4   = std::array<float, 4>;
using ivec4  = std::array<int, 4>;
using ivec2  = std::array<int, 2>;
using uvec2  = std::array<uint32, 2>;

template <auto>
struct MemberPtr;

template <typename T, typename M, M T::*P>
struct MemberPtr<P>
{
  static inline auto constexpr pmem = P;
  using class_t                     = std::decay_t<T>;
  using member_t                    = std::decay_t<M>;
};

union ScalarValue
{
  ivec2 ivalue2 = {0, 0};
  vec2  value2;
  float value;
  int   ivalue;
  bool  bvalue;

  inline ScalarValue(int a, int b) : ivalue2{a, b} {}
  inline ScalarValue(ivec2 v) : ivalue2(v) {}
  inline ScalarValue() {}
  inline ScalarValue(vec2 v) : value2(v) {}
  inline ScalarValue(float v) : value(v) {}
  inline ScalarValue(int v) : ivalue(v) {}
  inline ScalarValue(bool v) : bvalue(v) {}
};

struct Content
{
  std::unique_ptr<std::byte[]> data;
  size_t                       size = 0;

  Content()                              = default;
  Content(Content&&) noexcept            = default;
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
  static inline constexpr uint32_t k_null_32 = 0;
  static inline constexpr uint32_t mask      = 0x00ffffff;
  static inline constexpr uint32_t lifecycle = 0x01000000;

  constexpr handle() noexcept = default;
  constexpr handle(uint32_t v) noexcept : reserved(v) {}

  template <typename D>
  requires(std::derived_from<D, T> || std::derived_from<T, D>) constexpr handle(terra::handle<D> other) noexcept
      : reserved(other.reserved)
  {}

  constexpr operator uint32_t() const noexcept
  {
    return reserved;
  }

  constexpr operator bool() const noexcept
  {
    return reserved != 0;
  }

  constexpr auto operator<=>(handle const&) const noexcept = default;

  constexpr uint32_t index() const
  {
    return mask & reserved;
  }

  // unmasked value
  constexpr uint32_t um_index() const
  {
    return reserved;
  }

  constexpr handle cycle_up() const
  {
    return handle(reserved + lifecycle);
  }

public:
  uint32_t reserved = k_null_32;
};

using ghandle = handle<std::void_t<>>;

template <typename T>
struct HandleHash
{
  inline uint32_t operator()(handle<T> d) const noexcept
  {
    return d.reserved;
  }
};

template <typename T>
using optional_ref = std::optional<std::reference_wrapper<T>>;

class Pipeline;
class Node;

// default values recommended by http://isthe.com/chongo/tech/comp/fnv/
/// hash a single byte
constexpr uint32_t        Seed = 0x811C9DC5; // 2166136261
constexpr inline uint32_t fnv1a(unsigned char oneByte, uint32_t hash = Seed)
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
  auto          hexchar = [](char c) -> char8_t
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

enum class DataType
{
  eInvalid,
  eInt2,
  eFloat2,
  eInt,
  eFloat,
  eImage,
  eBuffer,
  eCurveData,
  eBool,
  eEnum
};

enum class Semantic
{
  eNone,
  eSource
};

struct DataFormat
{
  DataType type          = DataType::eInvalid;
  DataType scalarSubType = DataType::eInvalid;

  inline auto operator<=>(const DataFormat&) const noexcept = default;

  constexpr DataFormat() = default;
  constexpr DataFormat(DataType itype, DataType iscalarSubType = DataType::eFloat)
      : type(itype), scalarSubType(iscalarSubType)
  {}
  static bool isCompatible(DataFormat const& from, DataFormat const& to);
};

template <DataType Type, DataType Scalar = DataType::eFloat>
struct Format
{
  static inline constexpr DataType type   = Type;
  static inline constexpr DataType scalar = Scalar;

  static inline constexpr auto get()
  {
    return DataFormat(type, scalar);
  }
};

DataType         stringToType(std::string_view);
std::string_view typeToString(DataType);

union DataValue
{
  float fval;
  int   ival = 0;

  DataValue() = default;
  DataValue(float val) : fval(val) {}
  DataValue(int val) : ival(val) {}
};

struct LaunchParams
{
  ivec2   tileSize;
  float   frequency;
  float   wavelength;
  int32_t seed;
};

struct Box
{
  ivec2 offset{};
  ivec2 size{};

  inline constexpr auto operator<=>(Box const&) const noexcept = default;
};

// Rendering is done 1 tile at a time
struct EnvParams
{
  ivec2 tile{};
  // Current offset
  ivec2 startxy{};
  // Current tile size
  ivec2 tileSize{};
  // size of the output buffer
  ivec2 outputSize{};  
  // The region of data this param represents
  // relative to output
  Box region;
  // The region within the current tile
  Box tileRegion;

  float   frequency{};
  float   wavelength{};
  int32_t seed{};
  int32_t padding{};

  inline constexpr auto operator<=>(EnvParams const&) const noexcept = default;
};

enum class DrawHint
{
  eDefault,  // newline
  eSameline, // same line as the previous param
  eHidden
};

class DataSource;
using DataSourcePtr = std::shared_ptr<DataSource>;
using dshandle      = handle<DataSourcePtr>;
using DSHandleHash  = HandleHash<DataSourcePtr>;

struct Source
{
  dshandle source;
  uint32_t secondary = 0;
};

inline uintptr_t pack(uint32_t first, uint32_t sec)
{
  return (uintptr_t)first << 32ull | (uintptr_t)sec;
}

using uintpair = std::pair<uint32_t, uint32_t>;
inline uintpair unpack(uintptr_t v)
{
  return std::make_pair<uint32_t, uint32_t>(static_cast<uint32_t>(v >> 32ull), static_cast<uint32_t>(v & 0xffffffff));
}

using ThreadLocalAllocator = acl::linear_arena_allocator<>;

enum class PipelineType
{
  eGPU,
  eCPU
};


struct Event
{
  template <bool S>
  struct State
  {
    constexpr State() noexcept = default;
  };

  static inline constexpr auto iset   = State<true>();
  static inline constexpr auto iunset = State<true>();

  Event(State<true> = {}) {}
  Event(State<false>)
  {
    sem.acquire();
  }

  inline void reset()
  {
    sem.acquire();
  }

  inline void set()
  {
    sem.release();
  }

  inline void wait()
  {
    sem.acquire();
  }

  inline void waitAndSet()
  {
    wait();
    set();
  }

  std::binary_semaphore sem = std::binary_semaphore(1);
};

struct DisplayInfo
{
  std::u8string_view name;
  std::u8string_view help;
  std::u8string_view tooltip;

  const char* getName() const
  {
    return (const char*)name.data();
  }

  const char* getHelp() const
  {
    return (const char*)help.data();
  }

  const char* getTooltip() const
  {
    return (const char*)tooltip.data();
  }
  void from(std::string_view);
};

using WaitList = std::vector<std::future<void>>;

struct UVMeter
{
  vec2 offset{};
  vec2 recipSize{};
};

} // namespace terra
