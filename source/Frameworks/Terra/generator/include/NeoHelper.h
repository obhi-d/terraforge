#pragma once
#include <neo_script.hpp>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace terra
{

inline std::string_view getIdxParam(neo::command const& cmd, std::size_t i, std::string_view def = "")
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (size <= i)
    return def;
  return cmd.as_string(params.value()[i], def);
}

inline std::string getFirstConcat(neo::command const& cmd, std::string def = "")
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return def;
  auto const& l = params.value();
  if (size == 1)
    return std::string{cmd.as_string(l[0], def)};
  def = "";
  for (std::size_t i = 0; i < size; ++i)
  {
    def += cmd.as_string(l[i], "");
  }
  return def;
}

inline std::vector<std::string> getFirstList(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const&              l = params.value();
  std::vector<std::string> ret;
  if (size == 1)
  {
    if (l[0].index() == 2)
    {
      auto const& lst = std::get<2>(l[0]);
      for (auto const& v : lst)
      {
        ret.emplace_back(cmd.as_string(v, ""));
      }
      return ret;
    }
    else
      return {std::string{cmd.as_string(l[0], "")}};
  }

  for (std::size_t i = 0; i < size; ++i)
  {
    ret.emplace_back(cmd.as_string(l[i], ""));
  }
  return ret;
}

inline std::unordered_set<std::string> getFirstSet(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const& l = params.value();
  if (size == 1)
    return {std::string{cmd.as_string(l[0], "")}};
  std::unordered_set<std::string> ret;
  for (std::size_t i = 0; i < size; ++i)
  {
    ret.emplace(cmd.as_string(l[i], ""));
  }
  return ret;
}

}