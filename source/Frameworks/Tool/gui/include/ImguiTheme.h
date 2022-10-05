
#pragma once
#include "NeoHelper.h"
#include <filesystem>


namespace terra
{

struct ImguiTheme : neo::command_handler
{
  std::filesystem::path source;
  std::string           font;
  std::string           iconfont;
  std::string           iconpack;


};

#define ThemeCmdHandler(FnName, iObj, iState, iCmd) neo_cmd_handler(FnName, terra::ImguiTheme, iObj, iState, iCmd)

#define ThemeCmdEndHandler(FnName, iObj, iState, iName)                                                                \
  neo_cmdend_handler(FnName, terra::ImguiTheme, iObj, iState, iCmd)

#define ThemeTextHandler(FnName, iObj, iState, iType, iName, iContent)                                                 \
  neo_text_handler(FnName, terra::ImguiTheme, iObj, iState, iType, iName, iContent)

#define ThemeStarHandler(FnName, iObj, iState, iCmd) neo_star_handler(FnName, terra::ImguiTheme, iObj, iState, iCmd)

#define ThemeRegistry(name)      neo_registry(name)
#define ThemeRegister(name, reg) neo_register(name, reg)

#define ThemeStar(name)                     neo_star(name)
#define ThemeCmd(name)                      neo_cmd(name)
#define ThemeScopeDef(name)                 neo_scope_def(name)
#define ThemeScopeAuto(name)                neo_scope_auto(name)
#define ThemeScopeCust(name, end)           neo_scope_cust(name, end)
#define ThemeAliasid(par_scope, name, ex)   neo_aliasid(par_scope, name, ex)
#define ThemeSubaliasCust(name, end, alias) neo_subalias_cust(name, end, alias)
#define ThemeSubaliasDef(name, alias)       neo_subalias_def(name, alias)

#define ThemeSaveCurrent(as) neo_save_current(as)
#define ThemeSaveScope(as)   neo_save_scope(as)
#define ThemeFn(name)        neo_fn(name)

#define ThemeReadString(name)                                                                                          \
  ThemeCmdHandler(name, builder, state, cmd)                                                                           \
  {                                                                                                                    \
    builder.name = terra::getIdxParam(cmd, 0);                                                                         \
    return neo::retcode::e_success;                                                                                    \
  }


} // namespace terra