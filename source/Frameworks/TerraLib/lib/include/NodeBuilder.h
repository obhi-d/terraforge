#pragma once
#include <neo_script.hpp>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Node.h"

namespace terra
{
class Terra;
struct NodeCmdHandler : neo::command_handler
{
  template <typename L>
  NodeCmdHandler(NodeMeta& m, Terra& ctrl, L&& eh) : meta(m), controller(ctrl), errorHandler(std::forward<L>(eh))
  {}

  Terra&                           controller;
  std::u8string                    localizedString(std::string_view);
  std::function<void(std::string)> errorHandler;
  NodeMeta&                        meta;
};

#define NodeCmdHandler(FnName, iObj, iState, iCmd) neo_cmd_handler(FnName, terra::NodeCmdHandler, iObj, iState, iCmd)

#define NodeCmdEndHandler(FnName, iObj, iState, iName)                                                                 \
  neo_cmdend_handler(FnName, terra::NodeCmdHandler, iObj, iState, iCmd)

#define NodeTextHandler(FnName, iObj, iState, iType, iName, iContent)                                                  \
  neo_text_handler(FnName, terra::NodeCmdHandler, iObj, iState, iType, iName, iContent)

#define NodeStarHandler(FnName, iObj, iState, iCmd) neo_star_handler(FnName, terra::NodeCmdHandler, iObj, iState, iCmd)

#define NodeRegistry(name)      neo_registry(name)
#define NodeRegister(name, reg) neo_register(name, reg)

#define NodeStar(name)                     neo_star(name)
#define NodeCmd(name)                      neo_cmd(name)
#define NodeScopeDef(name)                 neo_scope_def(name)
#define NodeScopeAuto(name)                neo_scope_auto(name)
#define NodeScopeCust(name, end)           neo_scope_cust(name, end)
#define NodeAliasid(par_scope, name, ex)   neo_aliasid(par_scope, name, ex)
#define NodeSubaliasCust(name, end, alias) neo_subalias_cust(name, end, alias)
#define NodeSubaliasDef(name, alias)       neo_subalias_def(name, alias)

#define NodeSaveCurrent(as) neo_save_current(as)
#define NodeSaveScope(as)   neo_save_scope(as)
#define NodeFn(name)        neo_fn(name)

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

} // namespace terra