#pragma once
#include <string_view>

namespace terra
{

template <typename T>
class Property
{
public:
  Property(std::string_view n) : name(n) {}
  template <typename...Args>
  Property(std::string_view n, Args&&... v) noexcept : name(n), value(std::forward<Args>(v)...)
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
    return value <=> other;
  }

  inline bool operator!=(T const& other) const noexcept
  {
    return value != other;
  }

  
  inline bool operator==(T const& other) const noexcept
  {
    return value == other;
  }

  template <typename T>
  const char* getDisplayName(T& app) 
  {
    if (displayName.empty())
      displayName = app.getLocalizedString(name);
    return (const char*)displayName.data();
  }

private:
  std::string_view name;
  std::u8string_view displayName;

  T value = {};
};

} // namespace terra