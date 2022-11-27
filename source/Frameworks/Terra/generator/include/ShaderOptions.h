
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
  struct Options
  {
    inline bool operator==(Options const&) const noexcept = default;
    inline bool operator!=(Options const&) const noexcept = default;

    uint64_t mask = 0;
  };

  struct Dictionary
  {
    std::vector<std::string> names;
  };

  uint32_t dictionary = 0xffffffff;
  uint32_t node       = 0;
  Options  options    = {};

  inline ShaderOptions() noexcept {}
  inline ShaderOptions(uint32_t dict, uint32_t id) : dictionary(dict), node(id) {}

  void setOption(std::string_view option)
  {
    assert(dictionary);
    for (uint64_t i = 0; i < dictionary->names.size(); ++i)
    {
      if (option == dictionary->names[i])
      {
        options.mask |= 1ull << i;
        return;
      }
    }
  }

  void unsetOption(std::string_view option)
  {
    for (uint64_t i = 0; i < dictionary->names.size(); ++i)
    {
      if (option == dictionary->names[i])
      {
        options.mask &= ~(1ull << i);
        return;
      }
    }
  }

  void setOption(Options options)
  {
    options.mask |= options.mask;
  }

  void unsetOption(Options options)
  {
    options.mask &= ~options.mask;
  }

  void setOption(uint64_t idx)
  {
    options.mask |= 1ull << idx;
  }

  void unsetOption(uint64_t idx)
  {
    options.mask &= ~(1 << idx);
  }

  inline bool operator==(ShaderOptions const&) const noexcept = default;
  inline bool operator!=(ShaderOptions const&) const noexcept = default;

  struct hasher
  {
    inline uint32_t operator()(ShaderOptions const& option) const noexcept
    {
      return fnv1a(&option.options.mask, sizeof(option.options.mask));
    }
  };

  std::string_view name(uint32_t i) const
  {
    return optionDictionaries[dictionary].names[i];
  }

  int32_t value(uint64_t i) const
  {
    return options.mask & (1ull << i);
  }

  uint32_t size() const
  {
    return dictionary < optionDictionaries.size() ? (uint32_t)optionDictionaries[dictionary].names.size() : 0;
  }

  static std::vector<Dictionary> optionDictionaries;
};

} // namespace terra