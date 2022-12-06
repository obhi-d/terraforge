#define IMGUI_DEFINE_MATH_OPERATORS

#include "DrawableNode.h"
#include "DrawHelpers.h"
#include "NodeEditor.h"
#include "TerraMainApp.h"
#include "imgui_internal.h"

namespace terra
{

DrawableNode::DrawableNode(TerraMainApp& app, HDataSource id, ImVec2 pos)
{
  this->id  = id;
  this->pos = pos;
  // for nodes
  auto& source = get().get<DataSource>(id);

  output.id    = pack(id.um_index(), 0);
  output.flags = PinStateFlags::fOutput;
  imne::SetPinFlags(output.id, imne::PinKind::Output, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, true);

  switch (source.getType())
  {
  case DataSource::Type::eImage:
  case DataSource::Type::eCurve:
    style = app.getTheme().getNodeStyle("data");
    break;
  case DataSource::Type::eNode:
  {
    auto&       node = get().get<Node>(id);
    auto const& meta = node.meta;
    style            = app.getTheme().getNodeStyle(meta.style);

    // imne::SetNodeFlags(id.reserved, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, true);
    parameters.resize(meta.parameterDef.size());
    for (uint32 i = 0; i < (uint32)parameters.size(); ++i)
    {
      auto&       p = parameters[i];
      auto const& d = meta.parameterDef[i];
      p.id          = pack(id.um_index(), i + 1);
      if (d.format.type == DataTypeEnum::eInput || d.format.type == DataTypeEnum::eBuffer ||
          d.format.type == DataTypeEnum::ePostProcess || d.format.type == DataTypeEnum::eImage ||
          d.format.type == DataTypeEnum::eCurveData)
      {
        p.flags = PinStateFlags::fInputPin;
        imne::SetPinFlags(p.id, imne::PinKind::Input, imne::ImneObjFlags::ImneObjFlags_ExplicitInteractions, true);
      }
    }
  }
  break;
  }
  imne::SetNodePosition(id.reserved, pos);
}

DrawableNode::~DrawableNode()
{
  app().getDevice()->destroy(thumbnail);
}

void DrawableNode::drawPinIcon(NodeEditor& ne, NodeStyle const& style, PinData const& pin, DataFormat format,
                               bool filled)
{
  ImGui::SetCursorPos(pin.xy);

  auto cursorPos = ImGui::GetCursorScreenPos();
  auto drawList  = ImGui::GetWindowDrawList();

  IconType icon = IconType::Circle;
  switch (format.type)
  {
  case DataTypeEnum::eCurveData:
    icon = IconType::Grid;
    break;
  case DataTypeEnum::eImage:
    icon = IconType::Circle;
    break;
  case DataTypeEnum::eInput:
    icon = IconType::Flow;
    break;
  case DataTypeEnum::ePostProcess:
    icon = IconType::Square;
    break;
  case DataTypeEnum::eBuffer:
    icon = IconType::Diamond;
    switch (format.scalarSubType)
    {
    case DataTypeEnum::eFloat2:
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
    char idString[34] = {0}; // itoa can output 33 bytes maximum
    snprintf(idString, 33, "p%p", pin.id.AsPointer());

    ImGui::InvisibleButton(idString, ImVec2(style.pinSize * 1.1f, style.pinSize * 1.1f));
    auto color = style.pinColor;
    auto size  = style.pinSize;

    auto flags = imne::ImneObjFlags::ImneObjFlags_None;
    if (ImGui::IsItemActive())
      flags |= imne::ImneObjFlags::ImneObjFlags_IsActive;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
      if (ImGui::IsMouseDoubleClicked(1))
        flags |= imne::ImneObjFlags::ImneObjFlags_IsDoubleClicked;
      else if (ImGui::IsItemClicked())
        flags |= imne::ImneObjFlags::ImneObjFlags_IsClicked;
      else if (ne.acceptsAction())
      {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
          ne.showHelp(pin.id);
        else if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
          ne.showTooltip(pin.id);
      }
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

bool drawScalar(NodeStyle const& style, ParameterMeta const& def, DataTypeEnum type, ScalarValue& v)
{
  switch (type)
  {
  case DataTypeEnum::eInt2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragInt2(def.displayInfo.getName(), &v.ivalue2.x, 1.0f, def.values[ParameterMeta::eMin].ival,
                        def.values[ParameterMeta::eMax].ival))
      return true;
  }
  break;
  case DataTypeEnum::eInt:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::DragInt(def.displayInfo.getName(), &v.ivalue, 1.0f, def.values[ParameterMeta::eMin].ival,
                       def.values[ParameterMeta::eMax].ival))
      return true;
  }
  break;
  case DataTypeEnum::eFloat2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragFloat2(def.displayInfo.getName(), &v.value2.x, def.values[ParameterMeta::eStep].fval,
                          def.values[ParameterMeta::eMin].fval, def.values[ParameterMeta::eMax].fval))
      return true;
  }
  break;
  case DataTypeEnum::eFloat:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::DragFloat(def.displayInfo.getName(), &v.value, def.values[ParameterMeta::eStep].fval,
                         def.values[ParameterMeta::eMin].fval, def.values[ParameterMeta::eMax].fval))
      return true;
  }
  break;
  case DataTypeEnum::eBool:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::Checkbox(def.displayInfo.getName(), &v.bvalue))
      return true;
  }
  }
  return false;
}

void DrawableNode::drawParameter(NodeEditor& ne, NodeStyle const& style, Node& node, uint32_t i)
{
  ParameterMeta const& def   = node.meta.parameterDef[i];
  Parameter const&     param = node.param(i);
  auto&                pin   = parameters[i];
  switch (def.format.type)
  {
  case DataTypeEnum::eFloat:
  case DataTypeEnum::eFloat2:
  case DataTypeEnum::eInt:
  case DataTypeEnum::eInt2:
  case DataTypeEnum::eBool:
  {
    ScalarValue value = std::get<ScalarValue>(param);
    if (drawScalar(style, def, def.format.type, value))
      node.param(i, value);
  }
  break;
  case DataTypeEnum::eEnum:
    // draw combo
    {
      ScalarValue value = std::get<ScalarValue>(param);
      if (drawNodeEditorCombo(def.displayInfo.name, def.enumValues, value.ivalue2[0], value.ivalue2[1]))
        node.param(i, value);
      else
        node.state(i, value);
    }
    break;
  case DataTypeEnum::eCurveData:
    ImGui::TextUnformatted(ICON_FA_BEZIER_CURVE);
    ImGui::SameLine();
    ImGui::TextUnformatted(def.displayInfo.getName());
    break;
  case DataTypeEnum::eInput:
    ImGui::TextUnformatted(def.displayInfo.getName());
    ImGui::Dummy(ImVec2(4, 4));
    break;
  case DataTypeEnum::ePostProcess:
    ImGui::TextUnformatted(def.displayInfo.getName());
    break;
  case DataTypeEnum::eBuffer:
    if (std::holds_alternative<Source>(param))
    {
      ImGui::TextUnformatted(def.displayInfo.getName());
    }
    else
    {
      ScalarValue value = std::get<ScalarValue>(param);
      ImGui::SetNextItemWidth(style.fixedWidth);
      if (drawScalar(style, def, def.format.scalarSubType, value))
        node.param(i, value);
    }
    break;
  case DataTypeEnum::eImage:
  {
    ImGui::TextUnformatted(ICON_FA_FILE_IMAGE);
    ImGui::SameLine();
    ImGui::TextUnformatted(def.displayInfo.getName());
    break;
  }
  break;
  }
  if (ne.acceptsAction())
  {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
      ne.showHelp(pin.id);
    else if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
      ne.showTooltip(pin.id);
  }
}

void DrawableNode::updateThumbnailFromImage(Image& image)
{
  app().getDevice()->destroy(thumbnail);
  thumbnail = 0;
  if (!image.isLoaded())
    image.load();
  if (!image.isLoaded())
    return;
  //
  float du = 1.f / ThumbnailSize;

  float                           v       = 0;
  std::unique_ptr<std::uint8_t[]> sampled = std::make_unique<std::uint8_t[]>((int)ThumbnailSize * (int)ThumbnailSize);
  for (int y = 0; y < (int)ThumbnailSize; y++, v += du)
  {
    float u = 0;
    for (int x = 0; x < (int)ThumbnailSize; x++, u += du)
    {
      // nearest sampler
      sampled[x + y * (int)ThumbnailSize] = static_cast<std::uint8_t>(image.sample(u, v) * 255.f);
    }
  }
  thumbnail = app().getDevice()->createImage(
    GfxStorageClass::eStaticDeviceReadonly, (uint32_t)ThumbnailSize, (uint32_t)ThumbnailSize, ImageFormatEnum::eUnorm8,
    (ubyte_t const*)sampled.get(),
    GfxImage2D::Swizzle{GfxImage2D::eRed, GfxImage2D::eRed, GfxImage2D::eRed, GfxImage2D::eOne});
  thumbnailVersion = image.getVersion();
}

bool DrawableNode::begin(TerraMainApp& app, ImguiBackend& backend, NodeEditor& ne, uint32_t selectedStyle)
{
  auto&       source = get().get<DataSource>(id);
  auto const& style  = app.getTheme().getNodeStyle(selectedStyle ? selectedStyle - 1 : this->style);

  ImGui::PushID(id.um_index());
  imne::PushStyleColor(imne::StyleColor_NodeBg, style.nodeColor);
  imne::BeginNode(id.reserved);

  ImGui::BeginGroup();
  output.xy.y = ImGui::GetCursorPosY();

  // Output/Header

  switch (source.getType())
  {
  case DataSource::Type::eCurve:
    // todo Editable text
    ImGui::TextUnformatted((const char*)static_cast<CurveData&>(source).name.c_str());
    headerMaxY = ImGui::GetCursorPosY() + 2;
    if (drawCurveEditor(app, static_cast<CurveData&>(source)))
    {
      source.updateVersion();
    }
    break;
  case DataSource::Type::eImage:
  {
    // static const char* browseImage = "@browseImage"_lsc;
    auto name = static_cast<Image&>(source).source.filename().string();
    if (ImGui::Button(ICON_FA_FILE_IMAGE))
    {
      ne.changeImage(id);
    }

    if (!name.empty())
    {
      ImGui::SameLine();
      ImGui::TextUnformatted(name.c_str(), name.data() + name.length());
    }
    headerMaxY = ImGui::GetCursorPosY() + 2;
    if (thumbnailVersion != source.getVersion())
      updateThumbnailFromImage(static_cast<Image&>(source));
    break;
  }
  case DataSource::Type::eNode:
  {
    auto&       node = static_cast<Node&>(source);
    auto const& meta = node.meta;
    // todo Editable text
    ImGui::TextUnformatted((const char*)node.name.c_str());
    auto lastY = ImGui::GetItemRectSize().y;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
      ne.showHelp(imne::NodeId(id.reserved));
    else if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
      ne.showTooltip(imne::NodeId(id.reserved));

    ImGui::SameLine();
    auto padding = imne::GetStyle().NodePadding.y;
    ImGui::Dummy(ImVec2(style.pinSize * 1.5f, lastY + padding));

    headerMaxY = ImGui::GetCursorPosY();

    // Parameters
    for (uint32_t i = 0; i < node.getNumParams(); ++i)
    {
      parameters[i].xy.y = ImGui::GetCursorPosY();
      ImGui::Dummy(ImVec2(style.pinSize * 1.5f, 0));
      ImGui::SameLine();
      drawParameter(ne, style, node, i);
    }
  }
  break;
  }

  if (thumbnail)
  {
    ImGui::Image((ImTextureID)(std::uintptr_t)thumbnail.reserved, ImVec2{ThumbnailSize, ThumbnailSize});
  }

  return false;
}

void DrawableNode::end(TerraMainApp& app, ImguiBackend& backend, NodeEditor& ne, uint32_t selectedStyle)
{
  ImGui::EndGroup();
  auto min = ImGui::GetItemRectMin();
  auto max = ImGui::GetItemRectMax();

  auto const& style  = app.getTheme().getNodeStyle(selectedStyle ? selectedStyle - 1 : this->style);
  auto&       source = get().get<DataSource>(id);
  // Output
  output.xy.x = max.x - style.pinSize;
  drawPinIcon(ne, style, output, source.getFormat(), !source.isDetached());
  switch (source.getType())
  {
  case DataSource::Type::eNode:
  {
    auto&       node = static_cast<Node&>(source);
    auto const& meta = node.meta;
    // Parameters
    for (uint32_t i = 0; i < node.getNumParams(); ++i)
    {
      if (parameters[i].flags & PinStateFlags::fInputPin)
      {
        parameters[i].xy.x = min.x;
        auto src           = node.param(i);
        drawPinIcon(ne, style, parameters[i], meta.parameterDef[i].format,
                    std::holds_alternative<Source>(src) && std::get<Source>(src).source);
      }
    }
  }
  }
  imne::EndNode();
  imne::PopStyleColor();
  auto padding = imne::GetStyle().NodePadding.y * 0.5f;
  min          = imne::GetLastNodeDrawMin();
  max          = imne::GetLastNodeDrawMax();
  min.y -= padding;
  max.y = headerMaxY - padding;
  // Header
  drawHeader(ne, style, min, max);

  ImGui::PopID();
  firstDraw = false;
}

void DrawableNode::drawHeader(NodeEditor& ne, NodeStyle const& style, ImVec2 headerMin, ImVec2 headerMax)
{
  if (ImGui::IsItemVisible())
  {
    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

    auto drawList = imne::GetNodeBackgroundDrawList(id.reserved);

    const auto halfBorderWidth = imne::GetStyle().NodeBorderWidth * 0.5f;

    if ((headerMax.x > headerMin.x) && (headerMax.y > headerMin.y))
    {
      auto headerSeparatorMin = ImVec2(headerMin.x, headerMax.y);
      auto headerSeparatorMax = ImVec2(headerMax.x, headerMax.y);

      if ((headerSeparatorMax.x > headerSeparatorMin.x))
      {
        drawList->AddLine(headerSeparatorMin + ImVec2(-(8 - halfBorderWidth), -0.5f),
                          headerSeparatorMax + ImVec2((8 - halfBorderWidth), -0.5f),
                          ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
      }
    }
  }
}
} // namespace terra