
#pragma once
#include "NeoHelper.h"
#include <filesystem>
#include <array>
#include "imgui.h"
#include "glm/glm.hpp"
#include "IconsFontAwesome6.h"

namespace terra
{
enum ImageName
{
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

class Color
{
public:
  inline Color() = default;
  inline Color(uint32_t hexValue)
  {
    color.r = uint8_t((hexValue >> 24) & 0xFF); // Extract the RR byte
    color.g = uint8_t((hexValue >> 16) & 0xFF); // Extract the GG byte
    color.b = uint8_t((hexValue >> 8) & 0xFF);  // Extract the GG byte
    color.a = uint8_t((hexValue)&0xFF);         // Extract the BB byte
  }

  inline Color(ImVec4 f4)
  {
    color.r = (uint8_t)(f4.x * 255.f);            // Extract the RR byte
    color.g = (uint8_t)(f4.y * 255.f);            // Extract the GG byte
    color.b = (uint8_t)(f4.z * 255.f);            // Extract the GG byte
    color.a = (uint8_t)(f4.w * 255.f);            // Extract the BB byte
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
    return uint32_t{color.r} << 24 | uint32_t{color.g} << 16 | uint32_t{color.b} << 8 | uint32_t{color.a};
  }

  inline operator glm::vec4() const
  {
    return tovec4<glm::vec4>();
  }

  inline operator ImVec4() const
  {
    return tovec4<ImVec4>();
  }

private:
  template <typename T>
  T tovec4() const
  {
    return T{color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
  }

  glm::u8vec4 color = glm::u8vec4(255, 255, 255, 255);
};

struct ImThemeColors
{
  Color tint  = 0xffffffff;
  Color clear = 0xffffffff;
  Color text  = 0xffffffff;
  Color logo  = 0xffffffff;
  Color icon   = 0xff212121;
  Color iconHover = 0xff2f2f2f;
  Color iconPressed = 0xff2222ff;
};

struct ImguiTheme : neo::command_handler
{
  using Packs = std::array<ImagePack, ImagePackCount>;
  std::filesystem::path source;
  Packs                 images;
  ImThemeColors         themeColors;
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