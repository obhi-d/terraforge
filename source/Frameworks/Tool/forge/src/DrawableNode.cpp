#define IMGUI_DEFINE_MATH_OPERATORS

#include "DrawHelpers.h"
#include "TerraMainApp.h"
#include "DrawableNode.h"

namespace terra
{

DrawableNode::DrawableNode(TerraMainApp& app, hnode id, ImVec2 pos)
{
  this->id         = id;
  this->min        = pos;
  this->max        = ImVec2(pos.x + max.x, pos.y + max.y);
  auto        node = get().getNode(id);
  auto const& meta = node.getMeta();
  style            = app.getTheme().getNodeStyle(meta.style);
}



void DrawableNode::drawPinIcon(NodeStyle const& style, DataFormat format, bool detached) 
{
  auto cursorPos = ImGui::GetCursorScreenPos();
  auto drawList  = ImGui::GetWindowDrawList();

  IconType icon = IconType::Circle;
  switch (format.type)
  {
  case DataType::eImage:
    icon = IconType::Circle;
    break;
  case DataType::eDataSource:
    icon = IconType::Diamond;
    switch (format.scalarSubType)
    {
    case DataType::eFloat2:
      icon = IconType::RoundSquare;
      break;
    }
    break;

  }
  drawIcon(drawList, cursorPos, ImVec2(cursorPos.x + style.pinSize, cursorPos.y + style.pinSize), icon, detached,
           style.pinColor, style.pinFillColor);
}

bool DrawableNode::begin(TerraMainApp& app, ImguiBackend& backend, bool& previewNode)
{
  bool        changed = false;
  auto node = get().getNode(id);
  auto const& meta    = node.getMeta();
  auto const& style = app.getTheme().getNodeStyle(this->style);

  auto pinStartId = id.value() * 64;
  
  if (firstDraw)
    ImGui::BeginGroup();
  imne::BeginNode(id.reserved);

  
  if (backend.toggleButton(previewNode ? ICON_FA_CIRCLE_CHECK : ICON_FA_CIRCLE_STOP, previewNode,
                           ImVec2(20, 20), u8"") &&
      previewNode)
  {
    changed = true;
  }

  ImGui::TextUnformatted((const char*)node.getName().data());
    
  ImGui::PushID(id.reserved);
  auto outPin = pinStartId + 1;
  if (node.hasTextureOutput())
  {
    imne::BeginPin(outPin, imne::PinKind::Output);
    imne::PinPivotAlignment(ImVec2(1.0f, 0.5f));
    imne::PinPivotSize(ImVec2(0, 0));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width() - 4);
    drawPinIcon(style, node.getMeta().format, node.isDetached());
    imne::EndPin();
  }
  bool showTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort);
  bool showHelp    = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
  if (showTooltip || showHelp)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
    ImGui::BeginTooltip();
    if (showTooltip)
      ImGui::TextUnformatted((const char*)meta.brief.data());
    if (showHelp)
      ImGui::TextUnformatted((const char*)meta.help.data());
    ImGui::EndTooltip();
    ImGui::PopStyleVar();
  }
 
  return changed;
}

void DrawableNode::end(TerraMainApp& app, ImguiBackend& backend) 
{
  imne::EndNode();
  if (firstDraw)
    ImGui::EndGroup();
  min = ImGui::GetItemRectMin();
  max = ImGui::GetItemRectMax();
  ImGui::PopID();
}
}