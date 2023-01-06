#define IMGUI_DEFINE_MATH_OPERATORS

#include "NodeEditor.h"
#include "DrawHelpers.h"
#include "ImGuiFileDialog.h"
#include "ImguiBackend.h"
#include "ResourceUtils.h"
#include "TerraMainApp.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"
#include "imgui_node_editor_internal.h"
#include <filesystem>

namespace terra
{

void NodeEditor::init(TerraMainApp& app)
{
  cachedMetas.clear();
  CategoryMap names;
  terra::get().forEachMeta(
    [&names](auto const& name, uint32_t id, NodeMeta const& meta)
    {
      auto it = std::find_if(names.begin(), names.end(),
                             [&meta](auto const& entry)
                             {
                               return entry.first == meta.displayInfo.category;
                             });
      if (it != names.end())
        it->second.emplace_back(std::cref(meta));
      else
      {
        names.emplace_back();
        names.back().first = meta.displayInfo.category;
        names.back().second.emplace_back(std::cref(meta));
      }
    });

  cachedMetas = std::move(names);

  importNode         = app.localize("Editor.ImportNode");
  nodeEditor         = app.localize("Editor.Name");
  pasteNode          = app.localize("Editor.PasteNode");
  toggleSelectedNode = app.localize("Editor.ToggleSelectedNode");
  tipIncompatFormat  = app.localize("Editor.TipIncompatFormat");
  tipIncompatType    = app.localize("Editor.TipIncompatType");
  tipLink            = app.localize("Editor.TipLink");
  tipCreateNode      = app.localize("Editor.TipCreateNode");
  actions            = app.localize("Editor.Actions");
  imne::Config config;
  config.SettingsFile = "terra-nodes.json";
  editorContext       = imne::CreateEditor(&config);
  imne::SetCurrentEditor(editorContext);
  auto& style           = imne::GetStyle();
  style.NodeRounding    = 4.0f;
  style.PinRounding     = 2.0f;
  previewNodeStyle      = app.getTheme().getNodeStyle("selected") + 1;
  window.canBeMaximized = true;
  window.isMain         = true;
  window.locked         = false;
  window.maximized      = false;

  openPreview.name     = DisplayInfo(ICON_FA_MAGNIFYING_GLASS, "preview.help"_ls, "preview.tip"_ls);
  openPreview.function = [&app]()
  {
    app.getWindow().openPreview();
  };

  settingsWindow.name     = DisplayInfo(ICON_FA_GEAR, "settings.help"_ls, "settings.tip"_ls);
  settingsWindow.function = [&app]()
  {
    app.getWindow().openSettings();
  };
}

void NodeEditor::deinit(TerraMainApp& app)
{
  imne::DestroyEditor(editorContext);
  editorContext = nullptr;
  drawableNodes.clear();
  links.clear();
  cachedMetas.clear();
}

bool NodeEditor::drawNodeEditor(TerraMainApp& app, ImguiBackend& backend)
{
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
  imne::SetCurrentEditor(editorContext);

  setHeaderFont();
  if (ImGui::Begin((const char*)nodeEditor.data(), nullptr, window.locked ? ImGuiWindowFlags_NoMove : 0))
  {
    window.delegates.clear();
    if (!app.getWindow().isPreviewOpen())
      window.delegates.push_back(&openPreview);
    if (!app.getWindow().isSettingsOpen())
      window.delegates.push_back(&settingsWindow);
    switch (drawTitleMenu(window))
    {
    case WindowAction::eRestore:
      ImGui::RestoreWindow();
      break;
    case WindowAction::eMaximize:
      ImGui::MaximizeWindow();
      break;
    case WindowAction::eClose:
      // bail out
      ImGui::End();
      ImGui::PopStyleVar();
      return false;
    }
    setNormalFont();
    if (imne::Begin((const char*)nodeEditor.data(), ImVec2(0, 0)))
    {
      imne::Detail::EditorContext* ec = (imne::Detail::EditorContext*)(editorContext);
      auto                         rc = ec->GetViewRect();
      {
        auto newSize           = ImGui::GetWindowSize();
        auto openPopupPosition = ImGui::GetMousePos();
        imne::Suspend();
        // Menus
        if (imne::ShowBackgroundContextMenu())
          ImGui::OpenPopup("new_node");

        doContextMenu(app, openPopupPosition);
        executePendingAction(app);
        openImage(app);

        imne::Resume();
        doNodes(app, backend);
        {
          imne::NodeId id;
          if (imne::GetSelectedNodes(&id, 1) > 0 && id)
          {
            HDataSource nid = (uint32_t)(size_t)id;
            if (nid != previewNode && DataSource::isNode(nid))
            {
              nodeRegenRequired = true;
              previewNode       = nid;
            }
          }
        }
        imne::End();
        imne::SetCurrentEditor(nullptr);
      }
    }
    popNormalFont();
  }
  ImGui::End();
  popHeaderFont();
  ImGui::PopStyleVar();

  if (!nodeRegenRequired)
  {
    if (DataSource::isValid(previewNode) && previewNodeVersion != get().get<Node>(previewNode).getVersion())
      nodeRegenRequired = true;
  }

  if (nodeRegenRequired)
  {
    if (DataSource::isValid(previewNode))
      previewNodeVersion = get().get<Node>(previewNode).getVersion();
    else
      previewNodeVersion = 0;
    app.regenWithActor(previewNode);
    nodeRegenRequired = false;
  }
  return true;
}

void NodeEditor::drawScalar(TerraMainApp& app, ImguiBackend& backend, ParameterMeta const& def, Node& node,
                            uint32_t param)
{
  auto const& style = app.getTheme();
  auto        v     = std::get<ScalarValue>(node.param(param));
  bool        set   = false;
  static_assert(DataTypeEnum::kCount == 18);
  switch (def.format.scalarSubType)
  {
  case DataTypeEnum::eUint2:
  case DataTypeEnum::eInt2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragInt2(def.displayInfo.getName(), &v.ivalue2.x, 1.0f, def.ranges.minVal.ival, def.ranges.maxVal.ival))
      set = true;
  }
  break;
  case DataTypeEnum::eUint:
  case DataTypeEnum::eInt:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::DragInt(def.displayInfo.getName(), &v.ivalue, 1.0f, def.ranges.minVal.ival, def.ranges.maxVal.ival))
      set = true;
  }
  break;
  case DataTypeEnum::eFloat2:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragFloat2(def.displayInfo.getName(), &v.value2.x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
  }
  break;
  case DataTypeEnum::eFloat3:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragFloat3(def.displayInfo.getName(), &v.value3.x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
  }
  break;
  case DataTypeEnum::eFloat4:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragFloat4(def.displayInfo.getName(), &v.value4.x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
  }
  break;
  case DataTypeEnum::eMat4:
  {
    ImGui::SetNextItemWidth(style.fixedWidth * 2);
    if (ImGui::DragFloat4(def.displayInfo.getName(), &v.value4x4[0].x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
    if (ImGui::DragFloat4(def.displayInfo.getName(), &v.value4x4[1].x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
    if (ImGui::DragFloat4(def.displayInfo.getName(), &v.value4x4[2].x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
    if (ImGui::DragFloat4(def.displayInfo.getName(), &v.value4x4[3].x, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                          def.ranges.maxVal.fval))
      set = true;
  }
  break;
  case DataTypeEnum::eFloat:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::DragFloat(def.displayInfo.getName(), &v.value, def.ranges.stepVal.fval, def.ranges.minVal.fval,
                         def.ranges.maxVal.fval))
      set = true;
  }
  break;
  case DataTypeEnum::eBool:
  {
    ImGui::SetNextItemWidth(style.fixedWidth);
    if (ImGui::Checkbox(def.displayInfo.getName(), &v.bvalue))
      set = true;
  }
  }
  if (set)
    node.param(param, v);
}

void NodeEditor::drawParameter(TerraMainApp& app, ImguiBackend& backend, ParameterMeta const& def, Node& node,
                               uint32_t i)
{
  Parameter param = node.param(i);
  bool      isSrc = std::holds_alternative<Source>(param) && DataSource::isValid(std::get<Source>(param).source);
  static_assert(DataTypeEnum::kCount == 18);
  switch (def.format.type)
  {
  case DataTypeEnum::eFloat:
  case DataTypeEnum::eFloat2:
  case DataTypeEnum::eFloat3:
  case DataTypeEnum::eFloat4:
  case DataTypeEnum::eMat4:
  case DataTypeEnum::eInt:
  case DataTypeEnum::eInt2:
  case DataTypeEnum::eUint:
  case DataTypeEnum::eUint2:
  case DataTypeEnum::eBool:
  case DataTypeEnum::eArray:
  {
    drawScalar(app, backend, def, node, i);
  }
  break;
  case DataTypeEnum::eEnum:
    // draw combo
    {
      auto sv = std::get<ScalarValue>(node.param(i));
      if (ImGui::BeginCombo(def.displayInfo.getName(), def.enumDisplayInfo[sv.uvalue].getName()))
      {
        for (uint32_t e = 0; e < def.maxEnum; ++e)
        {
          bool selected = (e == sv.uvalue);
          if (ImGui::Selectable(def.enumDisplayInfo[i].getName(), selected))
          {
            if (i != sv.uvalue)
            {
              sv.uvalue = e;
              node.param(e, sv);
            }
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }
    break;
  case DataTypeEnum::eCurveData:

    if (isSrc)
    {
      ImGui::TextUnformatted(ICON_FA_BEZIER_CURVE);
      ImGui::SameLine();
      ImGui::TextUnformatted(def.displayInfo.getName());
      auto& cd = get().get<CurveData>(std::get<Source>(param).source);
      if (drawCurveEditor(app, cd))
        node.updateVersion();
    }

    break;
  case DataTypeEnum::eInput:
    break;
  case DataTypeEnum::ePostProcess:
    break;
  case DataTypeEnum::eBuffer:
    if (!isSrc)
      drawScalar(app, backend, def, node, i);
    break;
  case DataTypeEnum::eImage:
  {
    auto& id = get().get<Image>(std::get<Source>(param).source);
    if (ImGui::Button(ICON_FA_FILE_IMAGE))
    {
      changeImage(id.getSelf());
    }
    static float constexpr ThumbnailSize = 200.f;
    ImGui::SameLine();
    ImGui::TextUnformatted(def.displayInfo.getName());
    auto h = id.getHandle();
    if (h)
    {
      ImGui::Image((ImTextureID)(std::uintptr_t)h.reserved, ImVec2{ThumbnailSize, ThumbnailSize});
    }
    break;
  }
  break;
  }
}

void NodeEditor::drawNodeSettings(TerraMainApp& app, ImguiBackend& backend)
{
  if (previewNode && DataSource::isValid(previewNode))
  {
    auto&              node = get().get<Node>(previewNode);
    std::u8string_view category;
    bool               process = false;
    for (uint32_t i = 0, end = node.getNumParams(); i < end; ++i)
    {
      auto  param = node.meta.categorySorted[i];
      auto& def   = node.meta.parameterDef[param];
      if (!def.format.hidden)
      {
        if (def.displayInfo.category != category)
        {
          if (!category.empty() && process)
            ImGui::TreePop();
          category = def.displayInfo.category;
          process  = ImGui::TreeNode((const char*)category.data());
        }
        if (process)
          drawParameter(app, backend, def, node, param);
      }
    }
    if (!category.empty() && process)
      ImGui::TreePop();
  }
}

bool NodeEditor::acceptsAction()
{
  return pendingAction.action == Action::eNone;
}

void NodeEditor::showTooltip(PinData pin)
{
  pendingAction.action = Action::eShowTooltip;
  pendingAction.pin    = pin;
}

void NodeEditor::doContextMenu(TerraMainApp& app, ImVec2 openPopupPosition)
{
  auto const& theme = app.getTheme();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
  ImGui::PushItemWidth(-1);
  if (ImGui::BeginPopup("new_node", ImGuiWindowFlags_NoScrollbar))
  {
    if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0) && !frameCache.filterHasFocus)
    {
      ImGui::SetKeyboardFocusHere(0);
      frameCache.filterHasFocus = true;
    }
    if (ImGui::IsMouseClicked(1))
    {
      ImGui::CloseCurrentPopup();
    }
    else if (ImGui::InputText("##filter", frameCache.filterData.data(), frameCache.filterData.size(),
                              ImGuiInputTextFlags_EnterReturnsTrue) &&
             frameCache.createSelected)
    {
      pendingAction.meta     = frameCache.createSelected;
      pendingAction.action   = Action::eCreateNode;
      pendingAction.position = openPopupPosition;
      pendingAction.linkTo   = frameCache.linkTo;
      ImGui::CloseCurrentPopup();
    }
    else
    {
      if (ImGui::BeginChildFrame(ImGui::GetID("##items"), ImVec2(-1, 200), ImGuiWindowFlags_NoBackground))
      {
        std::u8string_view filter = (char8_t*)frameCache.filterData.data();
        for (auto& c : cachedMetas)
        {
          if (!c.second.empty())
          {
            ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)theme.themeColors.header);
            ImGui::TextUnformatted((const char*)c.first.data());
            ImGui::PopStyleColor();
            ImGui::Separator();
            {
              for (auto const& cr : c.second)
              {
                auto const& entry = cr.get();

                auto findStringIC = [](const std::u8string_view& strHaystack,
                                       const std::u8string_view& strNeedle) -> bool
                {
                  auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(),
                                        [](char8_t ch1, char8_t ch2)
                                        {
                                          return std::toupper(ch1) == std::toupper(ch2);
                                        });
                  return (it != strHaystack.end());
                };

                if (filter.empty() || findStringIC(entry.displayInfo.name, filter))
                {
                  if (!filter.empty() && !frameCache.createSelected)
                    frameCache.createSelected = &entry;
                  if (frameCache.createSelected == &entry)
                    ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)theme.themeColors.highlight);
                  if (ImGui::MenuItemEx(entry.displayInfo.getName(), app.getIcon(entry.icon).data()))
                  {
                    pendingAction.meta     = &entry;
                    pendingAction.action   = Action::eCreateNode;
                    pendingAction.position = openPopupPosition;
                    pendingAction.linkTo   = frameCache.linkTo;
                    ImGui::CloseCurrentPopup();
                  }
                  doTooltip(entry.displayInfo);
                  if (frameCache.createSelected == &entry)
                    ImGui::PopStyleColor();
                }
                else if (frameCache.createSelected == &entry)
                  frameCache.createSelected = nullptr;
              }
            }
          }
        }
        ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)theme.themeColors.header);
        ImGui::TextUnformatted((const char*)actions.data());
        ImGui::PopStyleColor();
        ImGui::Separator();
        if (ImGui::MenuItemEx((const char*)importNode.data(), ICON_FA_FILE_IMPORT))
        {
          pendingAction.action = Action::eImportNode;
          ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItemEx((const char*)pasteNode.data(), ICON_FA_PASTE))
        {
          pendingAction.action = Action::ePasteNode;
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::EndChildFrame();
    }
    ImGui::EndPopup();
    ImGui::PopItemWidth();
  }
  else
  {
    frameCache.linkTo         = PinData{};
    frameCache.filterHasFocus = false;
    frameCache.createSelected = nullptr;
    frameCache.filterData.fill(0);
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
}

void NodeEditor::openImage(TerraMainApp& app)
{
  auto const& theme = app.getTheme();
  if (loadImage("nodeImage", lastImagePath))
  {
    if (fileOpenData.action == Action::eChangeImage)
    {
      if (DataSource::isValid(fileOpenData.node))
      {
        get().get<Image>(fileOpenData.node).source = lastImagePath;
        get().get<Image>(fileOpenData.node).reload();
      }
    }
    else if (fileOpenData.action == Action::eImageData)
    {
      createImageNode(app, lastImagePath, fileOpenData.position);
      if (fileOpenData.linkTo.isValid())
      {
        setNextDataSource(theme.themeColors, drawableNodes.back()->getId(), pendingAction.linkTo);
      }
    }
    fileOpenData.action = Action::eNone;
    fileOpenData.node   = {};
    fileOpenData.linkTo = PinData{};
  }
}

void NodeEditor::executePendingAction(TerraMainApp& app)
{
  auto const& theme = app.getTheme();
  switch (pendingAction.action)
  {
  case Action::eChangeImage:
  case Action::eImageData:
    fileOpenData.action   = pendingAction.action;
    fileOpenData.linkTo   = pendingAction.linkTo;
    fileOpenData.node     = (uint32_t)pendingAction.node.Get();
    fileOpenData.position = pendingAction.position;
    ImGuiFileDialog::Instance()->OpenDialog("nodeImage", "Images", ".png", lastImagePath);
    break;
  case Action::eCurveData:
    createCurveEditor(app, pendingAction.position);
    if (pendingAction.linkTo.isValid())
    {
      setNextDataSource(theme.themeColors, drawableNodes.back()->getId(), pendingAction.linkTo);
    }
    break;
  case Action::eCreateNode:
    createNode(app, *pendingAction.meta, pendingAction.position);
    if (pendingAction.linkTo.isValid())
    {
      setNextDataSource(theme.themeColors, drawableNodes.back()->getId(), pendingAction.linkTo);
    }
    break;
  case Action::eShowTooltip:
  case Action::eShowHelp:
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::BeginTooltip();
    HelpInfo info;
    if (pendingAction.node && get().isValid((uint32_t)(size_t)pendingAction.node))
    {
      auto const& node = get().get<DataSource>((uint32_t)(size_t)pendingAction.node);
      info             = node.getHelpInfo(HelpType::eDataSource);
    }
    else if (pendingAction.pin.isValid())
    {
      auto nodel = pendingAction.pin.src();
      auto param = pendingAction.pin.id();
      if (get().isValid(nodel))
      {
        auto const& node = get().get<DataSource>(nodel);
        info = node.getHelpInfo(pendingAction.pin.isOutput() ? HelpType::eOutput : HelpType::eParameter, param);
      }
    }
    if (!info.tooltip.empty())
      ImGui::TextUnformatted(info.getTooltip(), (const char*)info.tooltip.data() + info.tooltip.length());
    if (pendingAction.action == Action::eShowHelp && !info.help.empty())
    {
      ImGui::TextUnformatted(info.getHelp(), (const char*)info.help.data() + info.tooltip.length());
    }
    ImGui::EndTooltip();
    ImGui::PopStyleVar();
    break;
  }
  case Action::eImportNode:
  case Action::ePasteNode:
  case Action::eNone:
    break;
  }
  pendingAction.action = Action::eNone;
  pendingAction.pin    = PinData{};
  pendingAction.meta   = nullptr;
  pendingAction.node   = {};
  pendingAction.linkTo = PinData{};
}

void NodeEditor::doNodes(TerraMainApp& app, ImguiBackend& backend)
{
  auto const& theme = app.getTheme();
  for (uint32_t n = 0; n < drawableNodes.size(); ++n)
  {
    auto& dn = *drawableNodes[n].get();

    bool toggled = previewNode == dn.getId();
    dn.begin(app, backend, *this, toggled ? previewNodeStyle : 0);
    dn.end(app, backend, *this, toggled ? previewNodeStyle : 0);
  }

  links.for_each(
    [&](auto& link)
    {
      imne::Link(link.id, link.start.pinId(), link.end.pinId(), toImgui(link.color), theme.linkThickness);
      return true;
    });

  // if (acceptsAction())
  {
    if (imne::BeginCreate(toImgui(theme.themeColors.link), theme.linkThickness))
    {
      auto showLabel = [](std::u8string_view label, Color color)
      {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
        auto size = ImGui::CalcTextSize((const char*)label.data());

        auto padding = ImGui::GetStyle().FramePadding;
        auto spacing = ImGui::GetStyle().ItemSpacing;

        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

        auto rectMin = ImGui::GetCursorScreenPos() - padding;
        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

        auto drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
        ImGui::TextUnformatted((const char*)label.data());
      };

      auto getFormat = [](PinData id) -> DataFormat
      {
        auto const& node = get().get<DataSource>(id.src());
        if (id.isOutput())
        {
          return node.getFormat(id.id());
        }
        else
          return static_cast<Node const&>(node).meta.parameterDef[id.id()].format;
      };

      imne::PinId startPinId = 0, endPinId = 0;
      if (imne::QueryNewLink(&startPinId, &endPinId))
      {
        auto startPin = PinData(startPinId);
        auto endPin   = PinData(endPinId);
        if (endPin.isOutput())
        {
          std::swap(startPin, endPin);
          std::swap(startPinId, endPinId);
        }

        if (startPin.isValid() && endPin.isValid())
        {
          if (endPin == startPin)
          {
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else if (endPin.isOutput() == startPin.isOutput())
          {
            showLabel(tipIncompatType, theme.themeColors.pinLabelReject);
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else if (!getFormat(endPin).isCompatible(getFormat(startPin)))
          {
            showLabel(tipIncompatFormat, theme.themeColors.pinLabelReject);
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else
          {
            showLabel(tipLink, theme.themeColors.pinLabelAccept);
            if (imne::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
            {
              createLink(theme.themeColors, startPin, endPin);
            }
          }
        }
      }

      imne::PinId pinId = 0;
      if (imne::QueryNewNode(&pinId))
      {
        if (pinId)
          showLabel(tipCreateNode, theme.themeColors.pinLabelAccept);

        if (imne::AcceptNewItem())
        {
          frameCache.linkTo = pinId;
          imne::Suspend();
          ImGui::OpenPopup("new_node");
          imne::Resume();
        }
      }
    }
    imne::EndCreate();
    if (imne::BeginDelete())
    {
      imne::LinkId linkId = 0;
      while (imne::QueryDeletedLink(&linkId))
      {
        if (imne::AcceptDeletedItem())
        {
          deleteLink(linkId);
        }
      }

      imne::NodeId nodeId = 0;
      while (imne::QueryDeletedNode(&nodeId))
      {
        if (imne::AcceptDeletedItem())
        {
          deleteNode(nodeId);
        }
      }
    }
    imne::EndDelete();
  }
}

void NodeEditor::createCurveEditor(TerraMainApp& app, ImVec2 pos)
{
  drawableNodes.emplace_back(std::make_unique<DrawableNode>(app, get().createCurve(), pos));
}

void NodeEditor::createImageNode(TerraMainApp& app, std::filesystem::path path, ImVec2 pos)
{
  drawableNodes.emplace_back(std::make_unique<DrawableNode>(app, get().getImage(std::move(path)), pos));
}

void NodeEditor::changeImage(HDataSource id)
{
  pendingAction.action = Action::eChangeImage;
  pendingAction.node   = id.um_index();
}

void NodeEditor::createNode(TerraMainApp& app, NodeMeta const& meta, ImVec2 pos)
{
  drawableNodes.emplace_back(std::make_unique<DrawableNode>(app, get().createNode(meta), pos));
  previewNode       = drawableNodes.back()->getId();
  nodeRegenRequired = true;
}

void NodeEditor::deleteNode(imne::NodeId node)
{
  for (auto dn = drawableNodes.begin(); dn != drawableNodes.end(); dn++)
  {
    if ((*dn)->is(HDataSource((uint32_t)node.Get())))
    {
      drawableNodes.erase(dn);
      break;
    }
  }
  get().destroy((uint32_t)(size_t)node);
}

void NodeEditor::createLink(ImThemeColors const& col, PinData start, PinData end)
{
  auto& dst    = get().get<Node>(end.src());
  Color color  = col.dsLink;
  auto  oldSrc = dst.param(end.id(), Source(start.src(), start.id()));
  if (std::holds_alternative<Source>(oldSrc))
  {
    auto     oldSrcHandle = std::get<Source>(oldSrc);
    uint32_t del          = 0;
    auto     pinStart     = PinData(PinData::output, oldSrcHandle.source.um_index(), oldSrcHandle.secondary);

    links.for_each(
      [&del, pinStart, end](auto const& l) -> bool
      {
        if ((l.start == pinStart && l.end == end) || (l.end == pinStart && l.start == end))
        {
          del = (uint32_t)l.id.Get();
          return false;
        }
        return true;
      });
    if (del)
      links.erase(del);
  }
  Link link;
  link.color      = color;
  link.start      = start;
  link.end        = end;
  auto id         = links.emplace(link);
  links.at(id).id = id.um_index();
}

void NodeEditor::deleteLink(imne::LinkId l)
{
  auto  addr = (uint32_t)l.Get();
  auto& lnk  = links.at(addr);

  auto& dst = get().get<Node>(lnk.end.src());

  dst.resetValue(lnk.end.id());
  links.erase(addr);
}

void NodeEditor::setNextDataSource(ImThemeColors const& col, HDataSource id, PinData src)
{
  auto&       node    = get().get<Node>(id);
  auto const& meta    = node.meta;
  auto const& srcNode = get().get<DataSource>(src.src());

  if (src.isOutput() || srcNode.getType() != DataSource::Type::eNode)
  {
    uint32_t paramChoice = (uint32_t)meta.parameterDef.size();
    uint32_t outIdx      = src.id();
    for (uint32_t i = 0; i < paramChoice; ++i)
    {
      if (meta.parameterDef[i].format.isCompatible(srcNode.getFormat(outIdx)))
      {
        paramChoice = i;
        break;
      }
    }
    if (paramChoice == (uint32_t)meta.parameterDef.size())
    {
      for (uint32_t i = 0; i < paramChoice; ++i)
      {
        if (meta.parameterDef[i].format == srcNode.getFormat())
        {
          paramChoice = i;
          break;
        }
      }
    }
    if (paramChoice == (uint32_t)meta.parameterDef.size())
      return;
    createLink(col, src, PinData(PinData::input, id, paramChoice));
  }
  else
  {
    for (uint32_t i = 0, end = (uint32_t)node.meta.outputs.size(); i < end; ++i)
    {
      if (node.getFormat(i).isCompatible(static_cast<Node const&>(srcNode).meta.parameterDef[src.id()].format))
      {
        createLink(col, PinData(PinData::output, id, i), src);
      }
    }
  }
}

void NodeEditor::tick() {}
} // namespace terra
