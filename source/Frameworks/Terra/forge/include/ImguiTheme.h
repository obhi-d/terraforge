
#pragma once

#include "IconsFontAwesome6.h"
#include "NeoHelper.h"
#include "Setup.h"
#include "glm/glm.hpp"
#include "imgui.h"
#include <array>
#include <filesystem>

namespace terra
{
enum ImageName
{
  // eHeaderFont,
  eFont,
  eIconFont,
  eLogo,
  eResize,
  kCount
};

inline ImTextureID toTexture(ImageName name)
{
  return (ImTextureID)(uintptr_t)name;
}

static inline constexpr uint32_t ImagePackCount = (uint32_t)ImageName::kCount;

struct ImagePack
{
  std::string path;
  glm::uvec2  size = glm::uvec2(0, 0);
};

struct ImThemeColors
{
  Color tint        = 0xffffffff;
  Color clear       = 0xffffffff;
  Color text        = 0xffffffff;
  Color logo        = 0xffffffff;
  Color icon        = 0xff212121;
  Color iconHover   = 0xff2f2f2f;
  Color iconPressed = 0xff2222ff;
  Color link        = 0xffffffff;
  Color texLink     = 0xaaaa11ff;
  Color dsLink      = 0x616666ff;
  Color header      = Color(155, 155, 155, 255);
  Color highlight   = Color(255, 155, 155, 255);
};

struct NodeStyle
{
  std::string name          = "default";
  float       fixedWidth    = 50.f;
  float       pinSize       = 16.0f;
  Color       pinColor      = 0xefefefef;
  Color       pinHoverColor = 0xffffffff;
  Color       pinFillColor  = 0xffffffff;
  Color       nodeColor     = 0x7d313aff;
  Color       titleHovered  = 0xaaaaaaaa;
  Color       titleSelected = 0xff111111;
  Color       textColor     = 0xffffffff;
  Color       textSelected  = 0xff1e62fe;
};

struct ImguiTheme : neo::command_handler
{
  using Packs = std::array<ImagePack, ImagePackCount>;
  std::filesystem::path  source;
  Packs                  images;
  ImThemeColors          themeColors;
  std::vector<NodeStyle> nodeStyles    = std::vector<NodeStyle>(1);
  float                  linkThickness = 2.0f;

  NodeStyle const& getNodeStyle(uint32_t s) const
  {
    return nodeStyles[s];
  }

  uint32_t getNodeStyle(std::string_view name) const
  {
    for (uint32_t i = 0; i < nodeStyles.size(); ++i)
    {
      if (nodeStyles[i].name == name)
        return i;
    }
    return 0;
  }
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