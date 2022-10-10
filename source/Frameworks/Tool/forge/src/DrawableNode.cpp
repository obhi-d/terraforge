#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui/imgui_internal.h"

#include "DrawHelpers.h"
#include "DrawableNode.h"
#include "TerraMainApp.h"

namespace terra
{

DrawableNode::DrawableNode(TerraMainApp& app, hnode id, ImVec2 pos)
{
  this->id         = id;
  this->pos        = pos;
  auto        node = get().getNode(id);
  auto const& meta = node.getMeta();
  style            = app.getTheme().getNodeStyle(meta.style);

  imne::SetNodeFlags(id.reserved, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, false);
  output.id    = id.value() * 64 + 1;
  output.flags = PinStateFlags::fOutput;
  imne::SetPinFlags(output.id, imne::PinKind::Output, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, true);

  parameters.resize(node.getNumParams());
  uint32_t paramId = id.value() * 64 + 2;
  for (uint32 i = 0; i < node.getNumParams(); ++i)
  {
    auto& p = parameters[i];
    auto& d = node.paramMeta(i);
    p.id    = paramId++;
    if (d.format.type == DataType::eDataSource || d.format.type == DataType::eImage)
    {
      p.flags = PinStateFlags::fInputPin;
      imne::SetPinFlags(p.id, imne::PinKind::Input, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, true);
    }
  }
  imne::SetNodePosition(id.reserved, pos);
}

void DrawableNode::drawPinIcon(NodeStyle const& style, PinData const& pin, DataFormat format, bool filled)
{
  ImGui::SetCursorPos(pin.xy);

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

  ImGui::SetCursorPos(pin.xy);
  imne::BeginPin(pin.id, pin.flags & PinStateFlags::fOutput ? imne::PinKind::Output : imne::PinKind::Input);
  {
    auto drawArea = ImVec2(20, 20);
    imne::PinPivotAlignment(ImVec2(1.0f, 0.5f));
    imne::PinPivotSize(ImVec2(0, 0));

    ImGui::SetCursorPos(pin.xy - ImVec2(style.pinSize * 0.05f, style.pinSize * 0.05f));
    ImGui::InvisibleButton("pin", ImVec2(style.pinSize * 1.1f, style.pinSize * 1.1f));
    auto color = style.pinColor;
    auto size  = style.pinSize;

    auto flags = imne::ImneObjFlags::ImneObjFlags_None;
    if (ImGui::IsItemActive())
      flags |= imne::ImneObjFlags::ImneObjFlags_IsActive;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
      if (ImGui::IsMouseDoubleClicked(1))
        flags |= imne::ImneObjFlags::ImneObjFlags_IsDoubleClicked;
      if (ImGui::IsItemClicked())
        flags |= imne::ImneObjFlags::ImneObjFlags_IsClicked;
      flags |= imne::ImneObjFlags::ImneObjFlags_IsHovered;
    }

    imne::SetPinInteraction(flags);

    if (ImGui::IsItemHovered())
    {
      color = style.pinHoverColor;
      size += 1;
    }

    drawIcon(drawList, cursorPos, ImVec2(cursorPos.x + size, cursorPos.y + size), icon, filled, color,
             filled ? style.pinFillColor : Color(0));
    imne::EndPin();
  }
}

void DrawableNode::drawParameter(NodeStyle const& style, Node& node, uint32_t i)
{
  ParameterMeta const& def   = node.paramMeta(i);
  Parameter&           param = node.param(i);
  auto&                pin   = parameters[i];
  switch (def.format.type)
  {
  case DataType::eInt2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    ivec2 v = std::get<ivec2>(param);
    if (ImGui::DragInt2((const char*)def.name.data(), v.data(), 1.0f, def.values[ParameterMeta::eMin].ival,
                        def.values[ParameterMeta::eMax].ival))
      node.setValue(i, Parameter(v));
  }
  break;
  case DataType::eInt:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    int v = std::get<int>(param);
    if (ImGui::DragInt((const char*)def.name.data(), &v, 1.0f, def.values[ParameterMeta::eMin].ival,
                       def.values[ParameterMeta::eMax].ival))
      node.setValue(i, Parameter(v));
  }
  break;
  case DataType::eFloat2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    vec2 v = std::get<vec2>(param);
    if (ImGui::DragFloat2((const char*)def.name.data(), v.data(), def.values[ParameterMeta::eStep].fval,
                          def.values[ParameterMeta::eMin].fval, def.values[ParameterMeta::eMax].fval))
      node.setValue(i, Parameter(v));
  }
  break;
  case DataType::eFloat:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    float v = std::get<float>(param);
    if (ImGui::DragFloat((const char*)def.name.data(), &v, def.values[ParameterMeta::eStep].fval,
                         def.values[ParameterMeta::eMin].fval, def.values[ParameterMeta::eMax].fval))
      node.setValue(i, Parameter(v));
  }
  break;
  case DataType::eBool:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    bool v = std::get<bool>(param);
    if (ImGui::Checkbox((const char*)def.name.data(), &v))
      node.setValue(i, Parameter(v));
  }
  break;
  case DataType::eDataSource:
  {
    DataSource v = std::get<DataSource>(param);
    if (v.node)
      ImGui::TextUnformatted((const char*)def.name.data());
    else
    {
      ImGui::SetNextItemWidth(style.fixedWidth);
      if (ImGui::DragFloat((const char*)def.name.data(), &v.constValue, def.values[ParameterMeta::eStep].fval,
                           def.values[ParameterMeta::eMin].fval, def.values[ParameterMeta::eMax].fval))
        node.setValue(i, Parameter(v));
    }
  }
  break;
  case DataType::eImage:
  {
    ImageSource v = std::get<ImageSource>(param);
  }
  break;
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    pin.flags = pin.flags | PinStateFlags::fShowTooltip;
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    pin.flags = pin.flags | PinStateFlags::fShowHelp;
}

bool DrawableNode::begin(TerraMainApp& app, ImguiBackend& backend, bool& previewNode)
{
  bool        changed = false;
  auto&        node    = get().getNode(id);
  auto const& meta    = node.getMeta();
  auto const& style   = app.getTheme().getNodeStyle(this->style);

  imne::BeginNode(id.reserved);

  ImGui::BeginGroup();
  if (backend.toggleButton(previewNode ? ICON_FA_CIRCLE_CHECK : ICON_FA_CIRCLE_STOP, previewNode, ImVec2(20, 20),
                           u8"") &&
      previewNode)
  {
    changed = true;
  }

  ImGui::PushID(id.reserved);

  ImGui::SameLine();

  // Output/Header

  output.xy.y = ImGui::GetCursorPosY();
  ImGui::TextUnformatted((const char*)node.getName().data());
  ImGui::SameLine();
  ImGui::Dummy(ImVec2(style.pinSize * 1.5f, 0));

  headerMaxY = ImGui::GetCursorPosY();

  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    output.flags = output.flags | PinStateFlags::fShowTooltip;
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    output.flags = output.flags | PinStateFlags::fShowHelp;

  // Parameters
  for (uint32_t i = 0; i < node.getNumParams(); ++i)
  {
    parameters[i].xy.y = ImGui::GetCursorPosY();
    ImGui::Dummy(ImVec2(style.pinSize * 1.5f, 0));
    ImGui::SameLine();
    drawParameter(style, node, i);
  }

  return changed;
}

void DrawableNode::end(TerraMainApp& app, ImguiBackend& backend)
{

  ImGui::EndGroup();
  auto min = ImGui::GetItemRectMin();
  auto max = ImGui::GetItemRectMax();

  auto        node  = get().getNode(id);
  auto const& meta  = node.getMeta();
  auto const& style = app.getTheme().getNodeStyle(this->style);

  // Output
  output.xy.x = max.x;
  drawPinIcon(style, output, node.getMeta().format, !node.isDetached());

  // Parameters
  for (uint32_t i = 0; i < node.getNumParams(); ++i)
  {
    if (parameters[i].flags & PinStateFlags::fInputPin)
    {
      parameters[i].xy.x = min.x;
      drawPinIcon(style, parameters[i], meta.parameterDef[i].format, parameters[i].flags & PinStateFlags::fIsFilled);
    }
  }

  imne::EndNode();
  auto padding = imne::GetStyle().NodePadding.y * 0.5f;
  min          = imne::GetLastNodeDrawMin();
  max          = imne::GetLastNodeDrawMax();
  min.y -= padding;
  max.y = headerMaxY - padding;
  // Header
  drawHeader(style, min, max);

  ImGui::PopID();
  firstDraw = false;
}

void DrawableNode::drawHeader(NodeStyle const& style, ImVec2 headerMin, ImVec2 headerMax)
{
  if (ImGui::IsItemVisible())
  {
    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

    auto drawList = imne::GetNodeBackgroundDrawList(id.reserved);

    const auto halfBorderWidth = imne::GetStyle().NodeBorderWidth * 0.5f;

    auto headerColor = style.title;
    if ((headerMax.x > headerMin.x) && (headerMax.y > headerMin.y))
    {

      drawList->AddRectFilled(headerMin - ImVec2(8 - halfBorderWidth, 4 - halfBorderWidth),
                              headerMax + ImVec2(8 - halfBorderWidth, 0), headerColor, imne::GetStyle().NodeRounding,
                              ImDrawFlags_RoundCornersTop);

      auto headerSeparatorMin = ImVec2(headerMin.x, headerMax.y);
      auto headerSeparatorMax = ImVec2(headerMax.x, headerMin.y);

      if ((headerSeparatorMax.x > headerSeparatorMin.x) && (headerSeparatorMax.y > headerSeparatorMin.y))
      {
        drawList->AddLine(headerSeparatorMin + ImVec2(-(8 - halfBorderWidth), -0.5f),
                          headerSeparatorMax + ImVec2((8 - halfBorderWidth), -0.5f),
                          ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
      }
    }
  }
}
} // namespace terra