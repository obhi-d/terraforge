#pragma once

#include "NeoHelper.h"
#include "hyb/HybridNodeMeta.h"

namespace terra
{
class Terra;
struct GpuScriptNodeBuilder : neo::command_handler
{
  template <typename L>
  GpuScriptNodeBuilder(NodeMeta& m, L&& eh) : meta(m), errorHandler(std::forward<L>(eh))
  {}

  std::u8string_view               localizedString(std::string_view);
  std::function<void(std::string)> errorHandler;
  GpuScriptNodeMeta&               meta;
};

#define NodeCmdExecute(FnName, iObj, iState, iCmd) neo_cmd_handler(FnName, terra::GpuScriptNodeBuilder, iObj, iState, iCmd)

#define NodeCmdEndHandler(FnName, iObj, iState, iName)                                                                 \
  neo_cmdend_handler(FnName, terra::GpuScriptNodeBuilder, iObj, iState, iCmd)

#define NodeTextHandler(FnName, iObj, iState, iType, iName, iContent)                                                  \
  neo_text_handler(FnName, terra::GpuScriptNodeBuilder, iObj, iState, iType, iName, iContent)

#define NodeStarHandler(FnName, iObj, iState, iCmd) neo_star_handler(FnName, terra::GpuScriptNodeBuilder, iObj, iState, iCmd)

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
} // namespace terra