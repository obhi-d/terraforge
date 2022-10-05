
#include "ImguiTheme.h"


ThemeTextHandler(notxt, build, state, type, name, content) {}

ThemeReadString(font);
ThemeReadString(iconfont);
ThemeReadString(iconpack);

ThemeRegistry(ThemeBuilder)
{
  neo_handle_text(notxt);

  ThemeCmd(font);
  ThemeCmd(iconfont);
  ThemeCmd(iconpack);
}

namespace terra
{}