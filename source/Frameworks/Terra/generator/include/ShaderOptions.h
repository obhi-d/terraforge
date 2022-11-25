
#pragma once
#include "Common.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace terra
{
struct ShaderOptions
{
  void setOption(std::string_view option, int32_t value)
  {
    auto id       = getIndex(option);
    auto pos      = options.end();
    bool posFound = false;
    for (auto it = options.begin(); it != options.end(); ++it)
    {
      if (it->first == id)
      {
        it->second = value;
        return;
      }
      else if (it->first > value && !posFound)
      {
        pos      = it;
        posFound = true;
      }
    }
    options.insert(pos, std::pair<uint32_t, int32_t>(id, value));
  }

  void removeOption(std::string_view option)
  {
    auto id = getIndex(option);
    for (auto it = options.begin(); it != options.end(); ++it)
    {
      if (it->first == id)
      {
        options.erase(it);
        return;
      }
    }
  }

  static uint32_t getIndex(std::string_view name)
  {
    auto it = optionIndices.find(name);
    if (it == optionIndices.end())
    {
      optionIndices.emplace(name, (uint32_t)optionIndices.size() + 1);
      return (uint32_t)optionIndices..size();
    }
    return *it;
  }

  inline bool operator==(ShaderOptions const&) const noexcept = default;
  inline bool operator!=(ShaderOptions const&) const noexcept = default;

  inline uint32_t operator() const noexcept
  {
    return fnv1a(options.data(), options.size() * sizeof(std::pair<uint32_t, int32_t>));
  }

  std::vector<std::pair<uint32_t, int32_t>>             options;
  static std::unordered_map<std::string_view, uint32_t> optionIndices;

} // namespace terra