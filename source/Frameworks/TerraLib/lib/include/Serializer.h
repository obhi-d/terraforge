
#pragma once
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>
#include <utility>

namespace terra
{
template <typename T>
concept VectorLike = requires(T& o)
{
  typename T::value_type;
  o.reserve(0);
  o.push_back(std::declval<typename T::value_type>());
};

template <typename T>
concept IntegerLike = std::is_integral_v<T> || std::is_enum_v<T>;

template <typename I>
static std::string numberToHex(I w, size_t hex_len = sizeof(I) << 1)
{
  static const char* digits = "0123456789ABCDEF";
  std::string        rc(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
    rc[i] = digits[(w >> j) & 0x0f];
  return rc;
}

template <typename T>
bool getFromDataStream(const std::vector<uint8_t>& dataStream, size_t& idx, T& value)
{
  if (dataStream.size() < idx + sizeof(T))
  {
    return false;
  }

  value = *reinterpret_cast<const T*>(dataStream.data() + idx);

  idx += sizeof(T);
  return true;
}

template <VectorLike Vector>
bool getFromDataStream(const std::vector<uint8_t>& dataStream, size_t& idx, Vector& value)
{
  uint32_t c = 0;
  if (!getFromDataStream(dataStream, idx, c))
    return false;
  value.reserve(c);
  for (size_t i = 0; i < c; i++)
  {
    typename Vector::value_type v;
    if (!getFromDataStream(dataStream, idx, v))
      return false;
    value.push_back(std::move(v));
  }
  return true;
}

template <IntegerLike T>
void addToDataStream(std::vector<uint8_t>& dataStream, T value)
{
  if constexpr (std::is_enum_v<T>)
  {
    auto evalue = static_cast<std::underlying_type_t<T>>(value);
    for (size_t i = 0; i < sizeof(T); i++)
    {
      dataStream.push_back((uint8_t)(evalue >> (i * 8)));
    }
  }
  else
  {
    for (size_t i = 0; i < sizeof(T); i++)
    {
      dataStream.push_back((uint8_t)(value >> (i * 8)));
    }
  }
}

template <VectorLike Vector>
void addToDataStream(std::vector<uint8_t>& dataStream, Vector const& value)
{
  uint32_t c = (uint32_t)value.size();
  addToDataStream(dataStream, c);
  for (size_t i = 0; i < value.size(); i++)
  {
    addToDataStream(dataStream, value[i]);
  }
}

template <typename T>
void addToDataStream(std::vector<uint8_t>& dataStream, std::span<T> value)
{
  uint32_t c = (uint32_t)value.size();
  addToDataStream(dataStream, c);
  for (size_t i = 0; i < value.size(); i++)
  {
    addToDataStream(dataStream, value[i]);
  }
}

inline void addToDataStream(std::vector<uint8_t>& dataStream, float value)
{
  uint32_t cast = *(uint32_t*)(&value);
  for (size_t i = 0; i < sizeof(value); i++)
  {
    dataStream.push_back((uint8_t)(cast >> (i * 8)));
  }
}

template <typename T, std::size_t N>
void addToDataStream(std::vector<uint8_t>& dataStream, std::array<T, N> value)
{
  for (size_t n = 0; n < N; ++n)
    addToDataStream(dataStream, value[n]);
}

} // namespace terra