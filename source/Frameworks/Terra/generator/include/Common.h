#pragma once
#include "generatorEnums.hpp"
#include <acl/dynamic_array.hpp>
#include <acl/linear_arena_allocator.hpp>
#include <acl/link.hpp>
#include <array>
#include <bit>
#include <cassert>
#include <cctype>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <glm/glm.hpp>
#include <memory>
#include <mimalloc-2.0/mimalloc.h>
#include <optional>
#include <semaphore>
#include <span>
#include <stdexcept>
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

using ubyte_t = std::uint8_t;

namespace detail
{
template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
// we specialize for pointers to member function
{
  enum
  {
    arity = sizeof...(Args)
  };
  // arity is the number of arguments.

  using result_type = ReturnType;

  template <size_t i>
  struct arg
  {
    using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
    // the i-th argument is equivalent to the i-th tuple element of a tuple
    // composed of those arguments.
  };

  template <size_t i>
  using arg_t = typename arg<i>::type;
};

template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)>
// we specialize for pointers to member function
{
  enum
  {
    arity = sizeof...(Args)
  };
  // arity is the number of arguments.

  using result_type = ReturnType;

  template <size_t i>
  struct arg
  {
    using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
    // the i-th argument is equivalent to the i-th tuple element of a tuple
    // composed of those arguments.
  };

  template <size_t i>
  using arg_t = typename arg<i>::type;
};

} // namespace detail

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
using vec2   = glm::vec2;
using vec3   = glm::vec3;
using vec4   = glm::vec4;
using mat4   = glm::mat4;
using ivec4  = glm::ivec2;
using ivec2  = glm::ivec2;
using uvec2  = glm::uvec2;

template <auto>
struct MemberPtr;

template <typename T, typename M, M T::*P>
struct MemberPtr<P>
{
  static inline auto constexpr pmem = P;
  using class_t                     = std::decay_t<T>;
  using member_t                    = std::decay_t<M>;
};

struct Content
{
  std::unique_ptr<ubyte_t[]> data;
  size_t                     size = 0;

  Content()                              = default;
  Content(Content&&) noexcept            = default;
  Content& operator=(Content&&) noexcept = default;
};

struct Blob
{
  std::vector<ubyte_t> content;

  inline void clear()
  {
    content.clear();
  }

  ubyte_t const* data() const
  {
    return content.data();
  }

  uint32_t size() const
  {
    return (uint32_t)content.size();
  }

  template <typename T>
  auto push(T const& data)
  {
    uint32_t s = (uint32_t)content.size();
    content.resize(s + sizeof(T));
    *(T*)(content.data() + s) = data;
    return s;
  }

  template <typename T>
  void replace(uint32_t offset, uint32_t size, T const& data)
  {
    if (size == sizeof(T))
    {
      at<T>(offset) = data;
    }
    else
    {
      content.erase(content.begin() + offset, content.begin() + offset + size);
      content.insert(content.begin() + offset, (ubyte_t const*)&data, ((ubyte_t const*)&data) + sizeof(T));
    }
  }

  template <typename T>
  T& at(uint32_t offset)
  {
    return reinterpret_cast<T&>(*(content.data() + offset));
  }

  template <typename T>
  T const& at(uint32_t offset) const
  {
    return reinterpret_cast<T const&>(*(content.data() + offset));
  }

  struct Reader
  {
    Blob const& blob;
    uint32_t    cursor = 0;

    Reader(Reader const& other) : blob(other.blob), cursor(other.cursor) {}
    Reader(Blob const& b) : blob(b) {}

    template <typename T>
    T const& read()
    {
      auto s = blob.content.data() + cursor;
      cursor += sizeof(T);
      return *(T const*)(s);
    }
  };

  Reader reader() const
  {
    return Reader(*this);
  }
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
    requires(std::derived_from<D, T> || std::derived_from<T, D>)
  constexpr handle(terra::handle<D> other) noexcept : reserved(other.reserved)
  {}

  constexpr explicit operator uint32_t() const noexcept
  {
    return reserved;
  }

  constexpr explicit operator bool() const noexcept
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
struct LinkHash
{
  inline uint32_t operator()(T d) const noexcept
  {
    return d.value();
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

template <typename string_type>
inline string_type parseU8(std::string_view from)
{
  string_type out;
  auto        hexchar = [](char c) -> char8_t
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

struct Semantic
{

  Semantic() = default;
  Semantic(std::string name) : id(fromString(std::move(name))) {}

  static uint16_t fromString(std::string name)
  {
    auto result = semanticMap.try_emplace(std::move(name), (uint16_t)semanticMap.size() + 1);
    return result.first->second;
  }

  inline explicit operator bool() const noexcept
  {
    return id != 0;
  }

  inline explicit operator uint32_t() const noexcept
  {
    return id;
  }

  inline auto operator<=>(Semantic const&) const noexcept = default;

  static std::unordered_map<std::string, uint16_t> semanticMap;
  static Semantic                                  heights;
  static Semantic                                  water;
  static Semantic                                  rocks;
  static Semantic                                  vegetation;

  uint16_t id = 0;
};

struct DataFormat
{
  Semantic          semantic      = {};
  DataTypeEnum      type          = DataTypeEnum::eInvalid;
  ImageFormatEnum   imageFormat   = ImageFormatEnum::eFloat;
  DataTypeEnum      scalarSubType = DataTypeEnum::eInvalid;
  ParamDeclTypeEnum declType      = ParamDeclTypeEnum::eNone;
  SamplerParamEnum  sampler       = SamplerParamEnum::eNone;
  uint8_t           maxArraySize  = 0;
  bool              preEval       = false;
  bool              hidden        = false;

  inline auto operator<=>(const DataFormat&) const noexcept = default;

  constexpr DataFormat() = default;
  constexpr DataFormat(DataTypeEnum itype, DataTypeEnum iscalarSubType = DataTypeEnum::eFloat,
                       ImageFormatEnum   format = ImageFormatEnum::eNone,
                       ParamDeclTypeEnum iindex = ParamDeclTypeEnum::eNone, Semantic isem = {},
                       SamplerParamEnum isampler = SamplerParamEnum::eNone, bool pre = false)
      : type(itype), scalarSubType(iscalarSubType), imageFormat(format), declType(iindex), semantic(isem),
        sampler(isampler), preEval(pre)
  {}

  inline constexpr bool isCompatible(DataFormat const& to) const
  {
    return type == to.type && scalarSubType == to.scalarSubType && imageFormat == to.imageFormat;
  }
};

template <DataTypeEnum Type, DataTypeEnum Scalar = DataTypeEnum::eFloat>
struct Format
{
  static inline constexpr DataTypeEnum type   = Type;
  static inline constexpr DataTypeEnum scalar = Scalar;

  static inline constexpr auto get()
  {
    return DataFormat(type, scalar);
  }
};

DataTypeEnum     stringToType(std::string_view);
std::string_view typeToString(DataTypeEnum);

union DataValue
{
  float fval;
  int   ival;

  DataValue() = default;
  DataValue(float val) : fval(val) {}
  DataValue(int val) : ival(val) {}
};

struct Box
{
  ivec2 offset{};
  ivec2 size{};

  inline constexpr auto operator<=>(Box const&) const noexcept = default;
};

enum class DrawHint
{
  eDefault,  // newline
  eSameline, // same line as the previous param
  eHidden
};

class DataSource;
using DataSourcePtr = std::shared_ptr<DataSource>;
using HDataSource   = handle<DataSourcePtr>;
using HHashSource   = HandleHash<DataSourcePtr>;

struct Source
{
  HDataSource source;
  uint32_t    secondary = 0;

  inline auto operator<=>(Source const&) const noexcept = default;

  inline Source() noexcept = default;
  inline Source(HDataSource d) : source(d) {}
  inline Source(HDataSource d, uint32_t v) : source(d), secondary(v) {}
};

struct SourceHash
{
  inline uint32_t operator()(Source d) const noexcept
  {
    return fnv1a(&d, sizeof(d));
  }
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

enum class HelpType
{
  eDataSource,
  eOutput,
  eParameter
};

struct HelpInfo
{
  std::u8string_view help;
  std::u8string_view tooltip;

  HelpInfo() = default;
  HelpInfo(std::u8string_view h, std::u8string_view t) : help(h), tooltip(t) {}

  const char* getHelp() const
  {
    return (const char*)help.data();
  }

  const char* getTooltip() const
  {
    return (const char*)tooltip.data();
  }
};

struct DisplayInfo : HelpInfo
{
  std::string_view   id;
  std::u8string_view name;
  std::u8string_view category;

  DisplayInfo() = default;
  DisplayInfo(std::string_view n, std::u8string_view h, std::u8string_view t)
      : id(n), name((char8_t const*)n.data(), n.length()), HelpInfo(h, t)
  {}
  DisplayInfo(std::u8string_view n, std::u8string_view h, std::u8string_view t) : name(n), HelpInfo(h, t) {}
  DisplayInfo(std::string_view);

  const char* getName() const
  {
    return (const char*)name.data();
  }

  void from(std::string_view);
};

using WaitList = std::vector<std::future<void>>;

struct UVMeter
{
  vec2 offset{};
  vec2 recipSize{};
};

struct MenuDelegate
{
  DisplayInfo           name;
  std::function<void()> function;
};

struct MenuData
{
  std::vector<MenuDelegate*> delegates;
  bool                       canBeMaximized = true;
  bool                       isMain         = false;
  bool                       locked         = false;
  bool                       maximized      = false;
  bool                       opened         = true;
};

constexpr inline float radians(float deg)
{
  return deg * 0.0174533f;
}

struct NoDomain
{};

struct Angle
{
  Angle() = default;
  Angle(float v) : degrees(v) {}

  float to_radians() const
  {
    return radians(degrees);
  }

  void clamp()
  {
    degrees = std::clamp(degrees, -180.f, 180.f);
  }

  operator float() const
  {
    return degrees;
  }

  float degrees = {};
};

struct Unorm
{
  Unorm() = default;
  Unorm(float v) : value(v) {}

  void clamp()
  {
    value = std::clamp(value, 0.f, 1.f);
  }

  operator float() const
  {
    return value;
  }

  float value = {};
};

struct Snorm
{
  Snorm() = default;
  Snorm(float v) : value(v) {}

  void clamp()
  {
    value = std::clamp(value, -1.f, 1.f);
  }

  operator float() const
  {
    return value;
  }

  float value = {};
};

using float16 = std::array<float, 16>;
using int16   = std::array<int, 16>;
using uint16  = std::array<uint32_t, 16>;

struct Rect
{
  glm::ivec2 offset = glm::vec2(0, 0);
  glm::ivec2 size   = glm::vec2(1, 1);

  inline bool operator==(Rect const&) const noexcept = default;
  inline bool operator!=(Rect const&) const noexcept = default;
};

struct Rotation
{
  // https://en.wikipedia.org/wiki/Spherical_coordinate_system
  float theta = 0.0f;   // 0 to 180
  float phi   = 180.0f; // 0 to 360

  Rotation() = default;
  Rotation(float the) : theta(the) {}
  Rotation(float the, float p) : theta(the), phi(p) {}

  void thetaAdd(float dt)
  {
    theta = std::clamp(theta + dt, 1.f, 179.f);
  }

  void phiAdd(float dt)
  {
    phi = std::clamp(phi + dt, 0.f, 360.f);
  }

  glm::vec3 toDir() const
  {
    auto theta    = glm::radians(this->theta);
    auto phi      = glm::radians(this->phi);
    auto sinTheta = std::sin(theta);
    auto cosTheta = std::cos(theta);
    auto sinPhi   = std::sin(phi);
    auto cosPhi   = std::cos(phi);
    return glm::normalize(glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi));
  }
};
// Color is ABGR, because of imgui
class Color
{
public:
  inline Color() = default;
  inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
  {
    color.r = r; // Extract the RR byte
    color.g = g; // Extract the GG byte
    color.b = b; // Extract the GG byte
    color.a = a; // Extract the BB byte
  }

  inline Color(uint32_t hexValue)
  {
    color.a = uint8_t((hexValue >> 24) & 0xFF); // Extract the RR byte
    color.b = uint8_t((hexValue >> 16) & 0xFF); // Extract the GG byte
    color.g = uint8_t((hexValue >> 8) & 0xFF);  // Extract the GG byte
    color.r = uint8_t((hexValue)&0xFF);         // Extract the BB byte
  }

  inline Color(float x, float y, float z, float w)
  {
    color.r = (uint8_t)(x * 255.f); // Extract the RR byte
    color.g = (uint8_t)(y * 255.f); // Extract the GG byte
    color.b = (uint8_t)(z * 255.f); // Extract the GG byte
    color.a = (uint8_t)(w * 255.f); // Extract the BB byte
  }

  inline Color(glm::vec4 f4)
  {
    color.r = (uint8_t)(f4.x * 255.f); // Extract the RR byte
    color.g = (uint8_t)(f4.y * 255.f); // Extract the GG byte
    color.b = (uint8_t)(f4.z * 255.f); // Extract the GG byte
    color.a = (uint8_t)(f4.w * 255.f); // Extract the BB byte
  }

  inline operator uint32_t() const
  {
    return uint32_t{color.a} << 24 | uint32_t{color.b} << 16 | uint32_t{color.g} << 8 | uint32_t{color.r};
  }

  inline operator glm::vec4() const
  {
    return tovec4<glm::vec4>();
  }

  inline float r() const
  {
    return color.r / 255.f;
  }

  inline float g() const
  {
    return color.g / 255.f;
  }

  inline float b() const
  {
    return color.b / 255.f;
  }

  inline float a() const
  {
    return color.a / 255.f;
  }

private:
  template <typename T>
  T tovec4() const
  {
    return T{color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
  }

  glm::u8vec4 color = glm::u8vec4(255, 255, 255, 255);
};

inline float distanceSq(vec2 a, vec2 b)
{
  auto v = a - b;
  return v.x * v.x + v.y * v.y;
}

inline vec2 scale(float a, vec2 b)
{
  return a * b;
}

inline vec2 add(vec2 a, vec2 b)
{
  return a + b;
}

inline vec2 sub(vec2 a, vec2 b)
{
  return a - b;
}

template <typename L>
bool forEachBit(L&& l, uint64_t params)
{
  int it = 0;
  while (params)
  {
    int s = std::countr_zero(params);
    it += (uint32_t)s;
    if (!l(it))
      return false;
    params >>= ((uint32_t)s + 1);
    it++;
  }
  return true;
}

using ArrayFloat    = std::vector<float>;
using ArrayInt      = std::vector<int>;
using ArrayUint     = std::vector<uint32_t>;
using ArrayFloatRef = std::shared_ptr<std::vector<float>>;
using ArrayIntRef   = std::shared_ptr<std::vector<int>>;
using ArrayUintRef  = std::shared_ptr<std::vector<uint32_t>>;

template <typename T>
std::span<ubyte_t const> toSpan(std::vector<T> const& ref)
{
  return std::span<ubyte_t const>((ubyte_t const*)ref.data(), ref.size() * sizeof(T));
}

} // namespace terra
