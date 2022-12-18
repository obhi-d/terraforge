
#pragma once

#include "Property.h"
#include "Setup.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <span>

namespace terra
{
class TerraMainApp;
struct CurveData;

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

bool         drawProp(TerraMainApp&, Property<glm::ivec2>&, int min, int max);
bool         drawProp(TerraMainApp&, Property<glm::uvec2>&, int min, int max);
bool         drawProp(TerraMainApp&, Property<int>&, int min, int max);
bool         drawProp(TerraMainApp&, Property<float>&, float min, float max, float step);
bool         drawProp(TerraMainApp&, Property<vec4>&, float min, float max, float step);
bool         drawProp(TerraMainApp&, Property<uint32_t>&, int min, int max);
bool         drawProp(TerraMainApp&, Property<Color>&);
bool         drawProp(TerraMainApp&, Property<TextureFile>&);
bool         drawProp(TerraMainApp& app, Property<bool>& prop);
int          doTooltip(DisplayInfo const& info, ImGuiHoveredFlags flags = 0);
bool         drawCurveEditor(TerraMainApp& app, CurveData& data);
bool         drawNodeEditorCombo(std::u8string_view name, std::span<std::u8string_view const> items, int& result,
                                 int& displayPopup);
bool         loadImage(const char* name, std::string& lastPath);
WindowAction drawTitleMenu(MenuData&);
void         setHeaderFont();
void         setNormalFont();
void         popHeaderFont();
void         popNormalFont();

} // namespace terra
