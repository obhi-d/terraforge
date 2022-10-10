
#pragma once

#include <imgui/imgui.h>

namespace terra
{
enum class IconType 
{
  Flow,
  Circle,
  Square,
  Grid,
  RoundSquare,
  Diamond
};

void drawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color,
              ImU32 innerColor);

}