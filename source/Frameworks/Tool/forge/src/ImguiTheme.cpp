
#include "ImguiTheme.h"
#include "glm/glm.hpp"

ThemeTextHandler(notxt, build, state, type, name, content) {}

constexpr std::array<std::string_view, terra::ImagePackCount> ImagePackNames = {"font", "iconfont", "logo", "resize"};

ThemeCmdHandler(tint, theme, state, cmd) 
{
  auto value = terra::getIdxParam(cmd, 0);
  uint32_t idx   = 0;
  std::from_chars(value.data(), value.data() + value.size(), idx, 16);
  theme.themeColors.tint = terra::Color(idx);
  return neo::retcode::e_success;
}
ThemeCmdHandler(clear, theme, state, cmd)
{
  auto     value = terra::getIdxParam(cmd, 0);
  uint32_t idx   = 0;
  std::from_chars(value.data(), value.data() + value.size(), idx, 16);
  theme.themeColors.clear = terra::Color(idx);
  return neo::retcode::e_success;
}
ThemeCmdHandler(pack, theme, state, cmd) 
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  terra::ImageName name   = terra::ImageName::eFont;
  for (auto const& p : params.value())
  {
    if (!std::holds_alternative<neo::single>(p))
      continue;
    auto const& param = std::get<neo::single>(p);
    if (param.name() == "index")
    {
      auto it = std::find(ImagePackNames.begin(), ImagePackNames.end(), param.value());
      if (it != ImagePackNames.end())
      {
        name = (terra::ImageName)std::distance(ImagePackNames.begin(), it);
      }      
    }
    else if ((uint32_t)name < terra::ImagePackCount)
    {
      if (param.name() == "file")
        theme.images[name].path = param.value();
      else if (param.name() == "x" || param.name() == "y")
      {
        uint32_t idx = 0;
        std::from_chars(param.value().data(), param.value().data() + param.value().size(), idx);
        if (param.name() == "x")
          theme.images[name].size.x = idx;
        else
          theme.images[name].size.y = idx;
      }
    }
  }
  return neo::retcode::e_success;
}

ThemeRegistry(ThemeBuilder)
{
  neo_handle_text(notxt);

  ThemeCmd(pack);
  ThemeCmd(tint);
  ThemeCmd(clear);
}

namespace terra
{}