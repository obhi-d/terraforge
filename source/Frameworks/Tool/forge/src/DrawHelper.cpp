
#include "CurveData.h"
#include "DrawHelpers.h"
#include "TerraMainApp.h"

#include "ImGuiFileDialog.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace terra
{
void drawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color,
              ImU32 innerColor)
{
  auto       rect           = ImRect(a, b);
  auto       rect_x         = rect.Min.x;
  auto       rect_y         = rect.Min.y;
  auto       rect_w         = rect.Max.x - rect.Min.x;
  auto       rect_h         = rect.Max.y - rect.Min.y;
  auto       rect_center_x  = (rect.Min.x + rect.Max.x) * 0.5f;
  auto       rect_center_y  = (rect.Min.y + rect.Max.y) * 0.5f;
  auto       rect_center    = ImVec2(rect_center_x, rect_center_y);
  const auto outline_scale  = rect_w / 24.0f;
  const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle

  if (type == IconType::Flow)
  {
    const auto origin_scale = rect_w / 24.0f;

    const auto offset_x  = 1.0f * origin_scale;
    const auto offset_y  = 0.0f * origin_scale;
    const auto margin    = (filled ? 2.0f : 2.0f) * origin_scale;
    const auto rounding  = 0.1f * origin_scale;
    const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
    // const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
    const auto canvas   = ImRect(rect.Min.x + margin + offset_x, rect.Min.y + margin + offset_y,
                                 rect.Max.x - margin + offset_x, rect.Max.y - margin + offset_y);
    const auto canvas_x = canvas.Min.x;
    const auto canvas_y = canvas.Min.y;
    const auto canvas_w = canvas.Max.x - canvas.Min.x;
    const auto canvas_h = canvas.Max.y - canvas.Min.y;

    const auto left     = canvas_x + canvas_w * 0.5f * 0.3f;
    const auto right    = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
    const auto top      = canvas_y + canvas_h * 0.5f * 0.2f;
    const auto bottom   = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
    const auto center_y = (top + bottom) * 0.5f;
    // const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

    const auto tip_top    = ImVec2(canvas_x + canvas_w * 0.5f, top);
    const auto tip_right  = ImVec2(right, center_y);
    const auto tip_bottom = ImVec2(canvas_x + canvas_w * 0.5f, bottom);

    drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
    drawList->PathBezierCubicCurveTo(ImVec2(left, top), ImVec2(left, top), ImVec2(left, top) + ImVec2(rounding, 0));
    drawList->PathLineTo(tip_top);
    drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
    drawList->PathBezierCubicCurveTo(tip_right, tip_right, tip_bottom + (tip_right - tip_bottom) * tip_round);
    drawList->PathLineTo(tip_bottom);
    drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
    drawList->PathBezierCubicCurveTo(ImVec2(left, bottom), ImVec2(left, bottom),
                                     ImVec2(left, bottom) - ImVec2(0, rounding));

    if (!filled)
    {
      if (innerColor & 0xFF000000)
        drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

      drawList->PathStroke(color, true, 2.0f * outline_scale);
    }
    else
      drawList->PathFillConvex(color);
  }
  else
  {
    auto triangleStart = rect_center_x + 0.32f * rect_w;

    auto rect_offset = -static_cast<int>(rect_w * 0.25f * 0.25f);

    rect.Min.x += rect_offset;
    rect.Max.x += rect_offset;
    rect_x += rect_offset;
    rect_center_x += rect_offset * 0.5f;
    rect_center.x += rect_offset * 0.5f;

    if (type == IconType::Circle)
    {
      const auto c = rect_center;

      if (!filled)
      {
        const auto r = 0.5f * rect_w / 2.0f - 0.5f;

        if (innerColor & 0xFF000000)
          drawList->AddCircleFilled(c, r, innerColor, 12 + extra_segments);
        drawList->AddCircle(c, r, color, 12 + extra_segments, 2.0f * outline_scale);
      }
      else
      {
        drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);
      }
    }

    if (type == IconType::Square)
    {
      if (filled)
      {
        const auto r  = 0.5f * rect_w / 2.0f;
        const auto p0 = rect_center - ImVec2(r, r);
        const auto p1 = rect_center + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
        drawList->AddRectFilled(p0, p1, color, 0, ImDrawFlags_RoundCornersAll);
#else
        drawList->AddRectFilled(p0, p1, color, 0, 15);
#endif
      }
      else
      {
        const auto r  = 0.5f * rect_w / 2.0f - 0.5f;
        const auto p0 = rect_center - ImVec2(r, r);
        const auto p1 = rect_center + ImVec2(r, r);

        if (innerColor & 0xFF000000)
        {
#if IMGUI_VERSION_NUM > 18101
          drawList->AddRectFilled(p0, p1, innerColor, 0, ImDrawFlags_RoundCornersAll);
#else
          drawList->AddRectFilled(p0, p1, innerColor, 0, 15);
#endif
        }

#if IMGUI_VERSION_NUM > 18101
        drawList->AddRect(p0, p1, color, 0, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale);
#else
        drawList->AddRect(p0, p1, color, 0, 15, 2.0f * outline_scale);
#endif
      }
    }

    if (type == IconType::Grid)
    {
      const auto r = 0.5f * rect_w / 2.0f;
      const auto w = ceilf(r / 3.0f);

      const auto baseTl = ImVec2(floorf(rect_center_x - w * 2.5f), floorf(rect_center_y - w * 2.5f));
      const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

      auto tl = baseTl;
      auto br = baseBr;
      for (int i = 0; i < 3; ++i)
      {
        tl.x = baseTl.x;
        br.x = baseBr.x;
        drawList->AddRectFilled(tl, br, color);
        tl.x += w * 2;
        br.x += w * 2;
        if (i != 1 || filled)
          drawList->AddRectFilled(tl, br, color);
        tl.x += w * 2;
        br.x += w * 2;
        drawList->AddRectFilled(tl, br, color);

        tl.y += w * 2;
        br.y += w * 2;
      }

      triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
    }

    if (type == IconType::RoundSquare)
    {
      if (filled)
      {
        const auto r  = 0.5f * rect_w / 2.0f;
        const auto cr = r * 0.5f;
        const auto p0 = rect_center - ImVec2(r, r);
        const auto p1 = rect_center + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
        drawList->AddRectFilled(p0, p1, color, cr, ImDrawFlags_RoundCornersAll);
#else
        drawList->AddRectFilled(p0, p1, color, cr, 15);
#endif
      }
      else
      {
        const auto r  = 0.5f * rect_w / 2.0f - 0.5f;
        const auto cr = r * 0.5f;
        const auto p0 = rect_center - ImVec2(r, r);
        const auto p1 = rect_center + ImVec2(r, r);

        if (innerColor & 0xFF000000)
        {
#if IMGUI_VERSION_NUM > 18101
          drawList->AddRectFilled(p0, p1, innerColor, cr, ImDrawFlags_RoundCornersAll);
#else
          drawList->AddRectFilled(p0, p1, innerColor, cr, 15);
#endif
        }

#if IMGUI_VERSION_NUM > 18101
        drawList->AddRect(p0, p1, color, cr, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale);
#else
        drawList->AddRect(p0, p1, color, cr, 15, 2.0f * outline_scale);
#endif
      }
    }
    else if (type == IconType::Diamond)
    {
      if (filled)
      {
        const auto r = 0.607f * rect_w / 2.0f;
        const auto c = rect_center;

        drawList->PathLineTo(c + ImVec2(0, -r));
        drawList->PathLineTo(c + ImVec2(r, 0));
        drawList->PathLineTo(c + ImVec2(0, r));
        drawList->PathLineTo(c + ImVec2(-r, 0));
        drawList->PathFillConvex(color);
      }
      else
      {
        const auto r = 0.607f * rect_w / 2.0f - 0.5f;
        const auto c = rect_center;

        drawList->PathLineTo(c + ImVec2(0, -r));
        drawList->PathLineTo(c + ImVec2(r, 0));
        drawList->PathLineTo(c + ImVec2(0, r));
        drawList->PathLineTo(c + ImVec2(-r, 0));

        if (innerColor & 0xFF000000)
          drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

        drawList->PathStroke(color, true, 2.0f * outline_scale);
      }
    }
    else
    {
      const auto triangleTip = triangleStart + rect_w * (0.45f - 0.32f);

      drawList->AddTriangleFilled(ImVec2(ceilf(triangleTip), rect_y + rect_h * 0.5f),
                                  ImVec2(triangleStart, rect_center_y + 0.15f * rect_h),
                                  ImVec2(triangleStart, rect_center_y - 0.15f * rect_h), color);
    }
  }
}

int doTooltip(DisplayInfo const& info, ImGuiHoveredFlags flags)
{
  int tip = 0;
  if (!info.tooltip.empty())
  {
    if ((!flags && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ||
        (flags && ImGui::IsWindowHovered(ImGuiHoveredFlags_DelayShort | flags)))
    {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted((const char*)info.tooltip.data());
      ImGui::EndTooltip();
      tip++;
    }
  }
  if (info.help.empty())
    return tip;
  if ((!flags && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ||
      (flags && ImGui::IsWindowHovered(ImGuiHoveredFlags_DelayShort | flags)))
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted((const char*)info.help.data());
    ImGui::EndTooltip();
    tip++;
  }
  return tip;
}

template <typename Prop>
bool doProp(bool result, Prop& prop)
{
  doTooltip(prop.getDisplayInfo());
  return result;
}

bool drawProp(TerraMainApp& app, Property<glm::ivec2>& prop, int min, int max)
{
  return doProp(ImGui::DragInt2(prop.getDisplayName(), &prop.get().x, 1.f, min, max), prop);
}
bool drawProp(TerraMainApp& app, Property<int>& prop, int min, int max)
{
  return doProp(ImGui::DragInt(prop.getDisplayName(), &prop.get(), 1.f, min, max), prop);
}
bool drawProp(TerraMainApp& app, Property<float>& prop, float min, float max, float step)
{
  return doProp(ImGui::DragFloat(prop.getDisplayName(), &prop.get(), step, min, max), prop);
}
bool drawProp(TerraMainApp& app, Property<Color>& prop)
{
  bool      result = false;
  glm::vec4 color  = prop.get();
  if (ImGui::ColorEdit4(prop.getDisplayName(), &color.x, ImGuiColorEditFlags_NoInputs))
  {
    prop   = Color(color);
    result = true;
  }
  // doTooltip(prop.getDisplayInfo());
  return result;
}

bool loadImage(const char* name, std::string& lastPath)
{
  bool accept = false;

  if (ImGuiFileDialog::Instance()->Display(name, 32, ImVec2{600, 400}))
  {
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      lastPath = ImGuiFileDialog::Instance()->GetFilePathName();
      accept   = true;
    }

    ImGuiFileDialog::Instance()->Close();
  }
  return accept;
}

bool drawProp(TerraMainApp& app, Property<TextureFile>& prop)
{
  bool  result = false;
  auto& file   = prop.get();
  if (file.image)
  {
    if (ImGui::ImageButton((ImTextureID)(uintptr_t)file.image, ImVec2{40, 40}))
      ImGuiFileDialog::Instance()->OpenDialog("propImage", "Images", ".png", prop.get().path);

    if (loadImage("propImage", prop.get().path))
    {
      result = prop.get().reload(app);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (40.f - ImGui::GetFontSize()) * 0.5f);
    ImGui::Text("%s", prop.getDisplayName());
  }
  doTooltip(prop.getDisplayInfo());
  return result;
}

bool drawNodeEditorCombo(std::u8string_view name, std::span<std::u8string_view const> items, int& result,
                         int& displayPopup)
{
  int iselect = result;
  if (ImGui::Button((const char*)items[result].data()))
  {
    displayPopup = true;
  }

  ImGui::SameLine();
  ImGui::TextUnformatted((const char*)name.data(), (const char*)(name.data() + name.length()));

  if (displayPopup) // kinda expensive with suspend resume, so do it if necessary
  {
    imne::Suspend();
    ImGui::OpenPopup((const char*)name.data());
    if (ImGui::BeginPopup((const char*)name.data(), ImGuiWindowFlags_Popup))
    {

      for (int i = 0; i < (int)items.size(); ++i)
      {
        bool selected = iselect == i;
        ImGui::PushID((void*)(intptr_t)i);
        ImGui::AlignTextToFramePadding();
        if (ImGui::Selectable((const char*)items[i].data(), selected, ImGuiSelectableFlags_SelectOnClick))
        {
          iselect = i;
          ImGui::CloseCurrentPopup();
          displayPopup = 0;
        }

        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
        if (selected)
          ImGui::SetItemDefaultFocus();
        ImGui::PopID();
      }

      bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
      if (displayPopup && !hovered && ImGui::IsMouseClicked(0))
      {
        ImGui::CloseCurrentPopup();
        displayPopup = 0;
      }
      ImGui::EndPopup();
    }
    else
      displayPopup = 0;

    imne::Resume();
  }

  if (result != iselect)
  {
    result = iselect;
    return true;
  }
  // ImGui::PopItemWidth();
  return false;
}

bool drawCurveEditor(TerraMainApp& app, CurveData& data)
{
  ImGuiContext& g = *GImGui;

  constexpr float ItemWidth  = 36;
  constexpr float Spacing    = 4;
  constexpr float Canvas     = 240;
  constexpr float Smoothing  = (Canvas * 2) / 3;
  constexpr float CurveWidth = 2;
  constexpr float GrabRadius = 8 / 2;
  constexpr float ControlSpc = 40;
  const ImColor   CircleColor(0.9f, 0.1f, 0.1f, 1.0f);
  const ImColor   CircleBorderColor(0.9f, 0.7f, 0.8f, 1.0f);
  const ImColor   HoveredCircleColor(0.1f, 0.1f, 0.8f, 1.0f);
  const ImColor   HoveredCircleBorderColor(0.8f, 0.8f, 0.5f, 1.0f);
  ImGuiStyle&     style = g.Style;

  data.beginEdit();

  auto& edits           = data.edits;
  auto& curve           = edits.spline;
  int   type            = 0;
  int   leftBound       = edits.left - 1;
  int   rightBound      = edits.right - 1;
  int   typeState       = edits.popupType;
  int   leftBoundState  = edits.popupLeftBound;
  int   rightBoundState = edits.popupRightBound;

  switch (curve.get_type())
  {
  case tk::spline<>::linear:
    type = 0;
    break;
  case tk::spline<>::cspline:
    type = 1;
    break;
  case tk::spline<>::cspline_hermite:
    type = 2;
    break;
  }
  float fixedX = ImGui::GetCursorPosX();
  float firstY = ImGui::GetCursorPosY() + 2;
  ImGui::PushID("#type");
  static std::array<std::u8string_view const, 3> curveTypes = {"@linearCurve"_ls, "@cubicCurve"_ls, "@hermiteCurve"_ls};
  static std::u8string_view                      typeName   = "@type"_ls;
  if (drawNodeEditorCombo(typeName, curveTypes, type, typeState))
  {
    edits.dirty = true;
  }
  ImGui::PopID();
  ImGui::SameLine();
  static const char* monotonic = "@monotonic"_lsc;
  if (ImGui::Checkbox(monotonic, &edits.monotonic))
  {
    edits.dirty = true;
  }
  ImGui::PushID("#left");
  static std::array<std::u8string_view const, 3> derivType = {"@first"_ls, "@second"_ls, "@notAKnot"_ls};
  static std::u8string_view                      derivName = "@leftDerivative"_ls;
  if (drawNodeEditorCombo(derivName, derivType, leftBound, leftBoundState))
  {
    edits.dirty = true;
  }
  ImGui::SameLine();
  ImGui::PushID("#value");
  ImGui::SetNextItemWidth(ItemWidth);
  if (ImGui::DragFloat("", &edits.leftValue, 0.01f, 0, 1, "%.2f"))
    edits.dirty = true;
  ImGui::PopID();
  ImGui::PopID();
  ImGui::PushID("#right");
  if (drawNodeEditorCombo(derivName, derivType, rightBound, rightBoundState))
  {
    edits.dirty = true;
  }
  ImGui::SameLine();
  ImGui::PushID("#value");
  ImGui::SetNextItemWidth(ItemWidth);
  if (ImGui::DragFloat("", &edits.rightValue, 0.01f, 0, 1, "%.2f"))
    edits.dirty = true;
  ImGui::PopID();
  ImGui::PopID();
  static const char* liveUpdate = "@liveUpdate"_lsc;
  if (ImGui::Checkbox(liveUpdate, &edits.liveUpdate))
  {
    edits.dirty = true;
  }

  ImDrawList*  DrawList = ImGui::GetWindowDrawList();
  ImGuiWindow* Window   = ImGui::GetCurrentWindow();
  ImVec2       canvas(Canvas, Canvas);

  ImGui::Dummy(ImVec2(0, 4));

  ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + canvas);
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, NULL))
    return false;

  auto          avail = ImGui::GetContentRegionAvail();
  const ImGuiID id    = Window->GetID(&data);

  ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
  ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
  ImGui::SetCursorScreenPos(bb.Min);
  ImGui::InvisibleButton("#f", canvas);
  // background grid
  auto inc = (int)(canvas.x / 4);
  for (int i = 0; i <= (int)canvas.x; i += inc)
  {
    DrawList->AddLine(ImVec2(bb.Min.x + i, bb.Min.y), ImVec2(bb.Min.x + i, bb.Max.y),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled));
  }

  inc = (int)(canvas.y / 4);
  for (int i = 0; i <= (int)canvas.y; i += inc)
  {
    DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y + i), ImVec2(bb.Max.x, bb.Min.y + i),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled));
  }

  DrawList->PushClipRect(bb.Min, bb.Max, true);
  // Curve
  ImColor         color(style.Colors[ImGuiCol_PlotLines]);
  float           lastY        = curve(0);
  float           lastX        = 0;
  constexpr float CurveSpacing = 1 / (Smoothing - 1);
  for (int i = 1; i < Smoothing; ++i)
  {
    float  currentX = i * CurveSpacing;
    float  currentY = curve(currentX);
    ImVec2 r(lastX * (bb.Max.x - bb.Min.x) + bb.Min.x, (1 - lastY) * (bb.Max.y - bb.Min.y) + bb.Min.y);
    ImVec2 s(currentX * (bb.Max.x - bb.Min.x) + bb.Min.x, (1 - currentY) * (bb.Max.y - bb.Min.y) + bb.Min.y);
    DrawList->AddLine(r, s, color, CurveWidth);
    lastX = currentX;
    lastY = currentY;
  }

  auto const& x = edits.cx;
  auto const& y = edits.cy;

  auto&  io             = ImGui::GetIO();
  ImVec2 mouse          = io.MousePos;
  char   control[]      = "#0p";
  bool   itemControlled = false;

  for (uint32_t i = 0; i < (uint32_t)x.size(); ++i)
  {
    ImVec2 p(x[i] * (bb.Max.x - bb.Min.x) + bb.Min.x, (1 - y[i]) * (bb.Max.y - bb.Min.y) + bb.Min.y);
    control[1] += i;
    auto distance = p - mouse;
    if (std::fabs(distance.x) < GrabRadius && std::fabs(distance.y) < GrabRadius || edits.dragged == (int)i)
    {
      DrawList->AddCircleFilled(p, GrabRadius, HoveredCircleColor);
      DrawList->AddCircle(p, GrabRadius + 1, HoveredCircleBorderColor);
      ImGui::SetTooltip("(%4.3f, %4.3f)", x[i], y[i]);
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      {
        itemControlled = true;
        edits.cx[i] += (io.MouseDelta.x / canvas.x);
        edits.cy[i] -= (io.MouseDelta.y / canvas.y);
        edits.dirty   = true;
        edits.dragged = (int)i;
        edits.cx[i]   = std::clamp(edits.cx[i], 0.0f, 1.0f);
        edits.cy[i]   = std::clamp(edits.cy[i], 0.0f, 1.0f);
      }
      else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
      {
        if (x.size() > 3)
        {
          edits.cx.erase(edits.cx.begin() + i);
          edits.cy.erase(edits.cy.begin() + i);
          edits.dirty = true;
        }
        else if (i == 1)
        {
          edits.cx[1] = 0.5f;
          edits.cy[1] = 0.5f;
          edits.dirty = true;
        }
      }
    }
    else
    {
      DrawList->AddCircleFilled(p, GrabRadius, CircleColor);
      DrawList->AddCircle(p, GrabRadius, CircleBorderColor);
    }
  }
  DrawList->PopClipRect();

  if (!itemControlled && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && bb.Contains(mouse))
  {
    float nx = (mouse.x - bb.Min.x) / (bb.Max.x - bb.Min.x);
    float ny = 1 - (mouse.y - bb.Min.y) / (bb.Max.y - bb.Min.y);

    auto bound = std::lower_bound(x.begin(), x.end(), nx);
    if (bound == x.end() || *bound != nx)
    {
      auto where = std::distance(x.begin(), bound);
      edits.cx.insert(bound, nx);
      edits.cy.insert(edits.cy.begin() + where, ny);
      edits.dirty = true;
    }
  }
  else if (itemControlled && edits.dragged >= 0 && edits.dragged < x.size())
  {
    // sort
    float xx = x[edits.dragged];
    float yy = y[edits.dragged];
    edits.cx.erase(edits.cx.begin() + edits.dragged);
    edits.cy.erase(edits.cy.begin() + edits.dragged);
    auto bound = std::lower_bound(x.begin(), x.end(), xx);
    if (bound != x.end() && *bound == xx)
    {
      auto where    = std::distance(x.begin(), bound);
      edits.dragged = (int)where;
    }
    else if (bound == x.end() || *bound != xx)
    {
      auto where = std::distance(x.begin(), bound);
      edits.cx.insert(bound, xx);
      edits.cy.insert(edits.cy.begin() + where, yy);
      edits.dragged = (int)where;
    }
  }

  edits.popupLeftBound  = leftBoundState != 0;
  edits.popupRightBound = rightBoundState != 0;
  edits.popupType       = typeState != 0;
  if (edits.dirty)
  {
    switch (type)
    {
    case 0:
      edits.type = tk::spline<>::linear;
      break;
    case 1:
      edits.type = tk::spline<>::cspline;
      break;
    case 2:
      edits.type = tk::spline<>::cspline_hermite;
      break;
    }

    if (leftBound == 2 && edits.cx.size() < 4)
      leftBound = 0;
    if (rightBound == 2 && edits.cx.size() < 4)
      rightBound = 0;

    rightBound += 1;
    leftBound += 1;

    edits.right = (tk::spline<>::bd_type)rightBound;
    edits.left  = (tk::spline<>::bd_type)leftBound;
  }

  ImGui::PopItemFlag();
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    return data.endEdits(true);
  return data.endEdits(false);
}
void popNormalFont()
{
  ImGui::PopFont();
  auto font   = ImGui::GetFont();
  font->Scale = 1.f;
}
void popHeaderFont()
{
  // ImGui::PopFont();
}
void setHeaderFont()
{
  auto font = ImGui::GetFont();
  // font->Scale = 1.5f;
  // ImGui::PushFont(font);
}

void setNormalFont()
{
  auto font   = ImGui::GetFont();
  font->Scale = .87f;
  ImGui::PushFont(font);
}

bool drawTitlebarButton(DisplayInfo const& text, float size)
{
  auto const& theme  = app().getTheme();
  auto        pos    = ImGui::GetCursorPos();
  auto        wpos   = ImGui::GetCursorScreenPos();
  bool        hover = ImGui::IsMouseHoveringRect(wpos, ImVec2(wpos.x + size, wpos.y + size));
  
  if (hover)
  {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
      ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)theme.themeColors.iconPressed);
    else
      ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)theme.themeColors.iconHover);
  }
  ImGui::SetCursorPos(pos);
  ImGui::Text(text.getName(), ImVec2(size + 4, size + 4));
   
  if (hover)
  {
    doTooltip(text, ImGuiHoveredFlags_AnyWindow);
    ImGui::PopStyleColor();
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      return true;
  }
  return false;
}

WindowAction drawTitleMenu(MenuData& state)
{
  WindowAction act            = WindowAction::eNone;
  float       width        = ImGui::GetWindowWidth();
  float       fontSize     = ImGui::GetFontSize();
  auto const& framePadding = ImGui::GetStyle().FramePadding;
  bool        canBeMaximized = (state.canBeMaximized && !ImGui::IsWindowDocked()) || state.isMain;
  float       size           = ((state.delegates.size() + (canBeMaximized ? 3 : 2)) * (fontSize + framePadding.x));
  float       titlebarHeight = fontSize + framePadding.y * 2.0f;
  auto        cursorPos      = ImGui::GetCursorPos();
  auto        pos            = ImGui::GetWindowPos();
  ImGui::PushClipRect(pos, ImVec2(pos.x + width, pos.y + titlebarHeight), false);
  size = width - size;
  for (auto& d : state.delegates)
  {
    ImGui::SetCursorPosX(size);
    ImGui::SetCursorPosY(framePadding.y);
    if (drawTitlebarButton(d->name, fontSize))
      d->function();
    size += (fontSize + framePadding.x);
  }

  ImGui::SetCursorPosX(size);
  ImGui::SetCursorPosY(framePadding.y);
  if (state.locked)
  {
    static DisplayInfo unlock(ICON_FA_LOCK, "unlock.help"_ls, "unlock.tip"_ls);
    if (drawTitlebarButton(unlock, fontSize))
      state.locked = false;
  }
  else
  {
    static DisplayInfo lock(ICON_FA_UNLOCK, "lock.help"_ls, "lock.tip"_ls);
    if (drawTitlebarButton(lock, fontSize))
      state.locked = true;
  }
  size += (fontSize + framePadding.x);

  if (canBeMaximized)
  {
    constexpr bool toogle = false;
    //  ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
    //  ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + width, pos.y + titlebarHeight));
    ImGui::SetCursorPosX(size);
    ImGui::SetCursorPosY(framePadding.y);
    if (state.maximized)
    {
      static DisplayInfo info(ICON_FA_WINDOW_RESTORE, "restore.help"_ls, "restore.tip"_ls);
      if (drawTitlebarButton(info, fontSize) || toogle)
      {
        state.maximized = false;
        act             = WindowAction::eRestore;
      }
    }
    else
    {
      static DisplayInfo info(ICON_FA_WINDOW_MAXIMIZE, "maximize.help"_ls, "maximize.tip"_ls);
      if (drawTitlebarButton(info, fontSize) || toogle)
      {
        state.maximized = true;
        act             = WindowAction::eMaximize;
      }
    }
    size += (fontSize + framePadding.x);
  }

  ImGui::SetCursorPosX(size);
  ImGui::SetCursorPosY(framePadding.y);
  static DisplayInfo info(ICON_FA_XMARK, "close.help"_ls, "close.tip"_ls);
  if( drawTitlebarButton(info, fontSize) )
    act = WindowAction::eClose;
  ImGui::PopClipRect();
  ImGui::SetCursorPos(cursorPos);
  return act;
}

} // namespace terra