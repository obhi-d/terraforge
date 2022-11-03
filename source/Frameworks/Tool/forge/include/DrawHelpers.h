
#pragma once

#include "Setup.h"
#include "Property.h"
#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace terra
{
class TerraMainApp;

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

bool drawProp(TerraMainApp&, Property<glm::ivec2>&, int min, int max);
bool drawProp(TerraMainApp&, Property<int>&, int min, int max);
bool drawProp(TerraMainApp&, Property<float>&, float min, float max, float step);
bool drawProp(TerraMainApp&, Property<Color>&);
bool drawProp(TerraMainApp&, Property<TextureFile>&);

}