#define IMGUI_DEFINE_MATH_OPERATORS

#include <filesystem>
#include "NodeEditor.h"
#include "ResourceUtils.h"
#include "ImguiBackend.h"
#include "imgui_node_editor.h"
#include "imgui_node_editor_internal.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "TerraMainApp.h"

namespace terra
{

void NodeEditor::init(TerraMainApp& app)
{
  for (auto const& dir_entry : std::filesystem::directory_iterator{getMediaPath() / "effects"})
  {
    if (dir_entry.is_regular_file() && dir_entry.path().has_extension() && dir_entry.path().extension() == ".ns")
      Terra::get().scanShader(dir_entry.path());
  }
  cachedMetas.clear();
  CategoryMap names;
  terra::get().forEachMeta(
    [&names](auto const& name, uint32_t id, NodeMeta const& meta)
    {
      auto it = std::find_if(names.begin(), names.end(),
                             [&meta](auto const& entry)
                             {
                               return entry.first == meta.category;
                             });
      if (it != names.end())
        it->second.emplace_back(std::cref(meta));
      else
      {
        names.emplace_back();
        names.back().first = meta.category;
        names.back().second.emplace_back(std::cref(meta));
      }
    });

  cachedMetas = std::move(names);

  importNode         = app.getLocalizedString("@Editor.ImportNode");
  nodeEditor         = app.getLocalizedString("@Editor.Name");
  pasteNode          = app.getLocalizedString("@Editor.PasteNode");
  toggleSelectedNode = app.getLocalizedString("@Editor.ToggleSelectedNode");
  tipIncompatFormat  = app.getLocalizedString("@Editor.TipIncompatFormat");
  tipIncompatType    = app.getLocalizedString("@Editor.TipIncompatType");
  tipLink            = app.getLocalizedString("@Editor.TipLink");
  tipCreateNode      = app.getLocalizedString("@Editor.TipCreateNode");
  actions            = app.getLocalizedString("@Editor.Actions");
  imne::Config config;
  config.SettingsFile = "terra-nodes.json";
  editorContext       = imne::CreateEditor(&config);
  imne::SetCurrentEditor(editorContext);
  auto& style        = imne::GetStyle();
  style.NodeRounding = 4.0f;
  style.PinRounding  = 2.0f;
}

void NodeEditor::drawNodeEditor(TerraMainApp& app, ImguiBackend& backend)
{
  imne::SetCurrentEditor(editorContext);
  if (ImGui::Begin((const char*)nodeEditor.data()))
  {
    imne::Begin((const char*)nodeEditor.data(), ImVec2(0, 0));
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

      imne::Resume();
      doNodes(app, backend);
      imne::End();
      imne::SetCurrentEditor(nullptr);
    }
  }
  ImGui::End();
}

void NodeEditor::doContextMenu(TerraMainApp& app, ImVec2 openPopupPosition)
{
  auto const& theme = app.getTheme();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
  if (ImGui::BeginPopup("new_node"))
  {

    ImGui::PushItemWidth(-1);
    if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0) && !frameCache.filterHasFocus)
    {
      ImGui::SetKeyboardFocusHere(0);
      frameCache.filterHasFocus = true;
    }
    if (ImGui::InputText("##filter", frameCache.filterData.data(), frameCache.filterData.size(),
      ImGuiInputTextFlags_EnterReturnsTrue) && frameCache.createSelected)
    {
      pendingAction.meta     = frameCache.createSelected;
      pendingAction.action   = Action::eCreateNode;
      pendingAction.position = openPopupPosition;
      pendingAction.linkTo   = frameCache.linkTo;
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopItemWidth();
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
            
            if (filter.empty() || entry.name.npos != entry.name.find(filter))
            {
              if (!filter.empty()  && !frameCache.createSelected)
                frameCache.createSelected = &entry;
              if (frameCache.createSelected == &entry)
                ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)theme.themeColors.highlight);
              if (ImGui::MenuItemEx((const char*)entry.name.data(), (const char*)entry.icon.c_str()))
              {
                pendingAction.meta     = &entry;
                pendingAction.action   = Action::eCreateNode;
                pendingAction.position = openPopupPosition;
                pendingAction.linkTo   = frameCache.linkTo;
              }
              if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
              {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted((const char*)entry.tooltip.data());
                ImGui::EndTooltip();
              }
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
    }

    if (ImGui::MenuItemEx((const char*)importNode.data(), ICON_FA_PASTE))
    {
      pendingAction.action = Action::ePasteNode;
    }
    ImGui::EndPopup();
  }
  else
  {
    frameCache.linkTo         = {};
    frameCache.filterHasFocus = false;
    frameCache.createSelected = nullptr;
    frameCache.filterData.fill(0);
  }
  ImGui::PopStyleVar();  
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
}

void NodeEditor::executePendingAction(TerraMainApp& app)
{
  auto const& theme = app.getTheme();
  switch (pendingAction.action)
  {
  case Action::eCreateNode:
    createNode(app, *pendingAction.meta, pendingAction.position);
    if (pendingAction.linkTo)
    {
      setNextDataSource(theme.themeColors, drawableNodes.back().getId(), pendingAction.linkTo);
    }
    break;
  case Action::eShowTooltip:
  case Action::eShowHelp:
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::BeginTooltip();
    if (pendingAction.node && get().isValid((uint32_t)(size_t)pendingAction.node))
    {
      auto const& node = get().getNode((uint32_t)(size_t)pendingAction.node);
      ImGui::TextUnformatted((const char*)node.getMeta().tooltip.data());
    }
    else if (pendingAction.pin)
    {
      auto [nodel, param] = unpack((size_t)pendingAction.pin);
      if (get().isValid(nodel))
      {
        auto const& node = get().getNode(nodel);
        if (param == 0)
          ImGui::TextUnformatted((const char*)node.getMeta().tooltip.data());
        else
          ImGui::TextUnformatted((const char*)node.paramMeta(param - 1).tooltip.data());
      }
    }
    if (pendingAction.action == Action::eShowHelp)
    {
      if (pendingAction.node && get().isValid((uint32_t)(size_t)pendingAction.node))
      {
        auto const& node = get().getNode((uint32_t)(size_t)pendingAction.node);
        if (!node.getMeta().help.empty())
          ImGui::TextUnformatted((const char*)node.getMeta().help.data());
      }
      else if (pendingAction.pin)
      {
        auto [nodel, param] = unpack((size_t)pendingAction.pin);
        if (get().isValid(nodel))
        {
          auto const& node = get().getNode(nodel);
          if (param == 0)
          {
            if (!node.getMeta().help.empty())
              ImGui::TextUnformatted((const char*)node.getMeta().help.data());
          }
          else
          {
            if (!node.paramMeta(param - 1).help.empty())
              ImGui::TextUnformatted((const char*)node.paramMeta(param - 1).help.data());
          }
        }
      }
    }
    ImGui::EndTooltip();
    ImGui::PopStyleVar();
    break;
  }
  }
  pendingAction.action = Action::eNone;
  pendingAction.pin    = {};
  pendingAction.meta   = nullptr;
  pendingAction.node   = {};
  pendingAction.linkTo = {};
}

void NodeEditor::doNodes(TerraMainApp& app, ImguiBackend& backend)
{
  auto const& theme = app.getTheme();
  for (uint32_t n = 0; n < drawableNodes.size(); ++n)
  {
    auto& dn = drawableNodes[n];

    bool toggled = previewNode == n;
    if (dn.begin(app, backend, *this, toggled))
    {
      previewNode          = n;
      nodeSelectionChanged = true;
    }
    dn.end(app, backend, *this);
  }

  links.for_each(
    [&](auto& link)
    {
      imne::Link(link.id, link.start, link.end, link.color, theme.linkThickness);
      return true;
    });

  // if (acceptsAction())
  {
    uintpair newLinkPin;
    if (imne::BeginCreate(theme.themeColors.link, theme.linkThickness))
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

      auto getFormat = [](uintpair id) -> DataFormat const&
      {
        auto const& node = get().getNode(id.first);
        if (!id.second)
          return node.getFormat();
        return node.paramMeta(id.second - 1).format;
      };

      imne::PinId startPinId = 0, endPinId = 0;
      if (imne::QueryNewLink(&startPinId, &endPinId))
      {
        auto startPin = unpack((size_t)startPinId);
        auto endPin   = unpack((size_t)endPinId);

        newLinkPin = startPinId ? startPin : endPin;

        if (startPin.second)
        {
          std::swap(startPin, endPin);
          std::swap(startPinId, endPinId);
        }

        if (startPinId && endPinId)
        {
          if (endPin == startPin)
          {
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else if (endPin.second && startPin.second)
          {
            showLabel(tipIncompatType, Color(45, 32, 32, 180));
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else if (getFormat(endPin) != getFormat(startPin))
          {
            showLabel(tipIncompatFormat, Color(45, 32, 32, 180));
            imne::RejectNewItem(ImColor(255, 0, 0), theme.linkThickness);
          }
          else
          {
            showLabel(tipLink, Color(32, 45, 32, 180));
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
          showLabel(tipCreateNode, Color(32, 45, 32, 180));

        if (imne::AcceptNewItem())
        {
          frameCache.linkTo = pinId;
          imne::Suspend();
          ImGui::OpenPopup("new_node");
          imne::Resume();
        }
      }
    }
    else
      newLinkPin = {0, 0};

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

void NodeEditor::createNode(TerraMainApp& app, NodeMeta const& meta, ImVec2 pos)
{
  drawableNodes.emplace_back(app, get().createNode(meta), pos);
}

void NodeEditor::deleteNode(imne::NodeId node)
{
  for (auto dn = drawableNodes.begin(); dn != drawableNodes.end(); dn++)
  {
    if (dn->is(hnode((uint32_t)node.Get())))
    {
      drawableNodes.erase(dn);
      break;
    }
  }
  get().destroy((uint32_t)(size_t)node);
}

void NodeEditor::createLink(ImThemeColors const& col, uintpair start, uintpair end)
{
  auto& src   = get().getNode(start.first);
  auto& dst   = get().getNode(end.first);
  Color color = src.hasTextureOutput() ? col.texLink : col.dsLink;
  hnode oldSrc = src.hasTextureOutput() ? dst.setValue(end.second - 1, ImageSource(hnode(start.first)))
                                        : dst.setValue(end.second - 1, DataSource(hnode(start.first)));
  if (oldSrc)
  {
    uint32_t del = 0;
    imne::PinId pinStart = pack(oldSrc, 0);
    imne::PinId pinEnd = pack(end.first, end.second);

    links.for_each([&del, pinStart, pinEnd](auto const& l) -> bool 
      {
        if (l.start== pinStart && l.end == pinEnd)
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
  link.start      = pack(start.first, start.second);
  link.end        = pack(end.first, end.second);
  auto id         = links.emplace(link);
  links.at(id).id = id;
}

void NodeEditor::deleteLink(imne::LinkId l)
{
  auto  addr  = (uint32_t)l.Get();
  auto& lnk   = links.at(addr);
  auto  start = unpack(lnk.start.Get());
  auto  end   = unpack(lnk.end.Get());

  auto& src = get().getNode(start.first);
  auto& dst = get().getNode(end.first);

  if (src.hasTextureOutput())
    dst.setValue(end.second - 1, ImageSource(hnode()));
  else
    dst.setValue(end.second - 1, DataSource(hnode()));

  links.erase(addr);
}

void NodeEditor::setNextDataSource(ImThemeColors const& col, hnode id, imne::PinId src)
{
  auto & node    = get().getNode(id);
  auto const& meta    = node.getMeta();
  auto        srcPin  = unpack(src.Get());
  auto const& srcNode = get().getNode(srcPin.first);
 
  if (!srcPin.second)
  {
    uint32_t paramChoice = (uint32_t)meta.parameterDef.size();
    for (uint32_t i = 0; i < paramChoice; ++i)
    {
      if (meta.parameterDef[i].name == "Source" && meta.parameterDef[i].format == srcNode.getFormat())
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
    createLink(col, srcPin, uintpair(id, paramChoice + 1));
  }
  else
  {
    if (node.getFormat() == srcNode.getMeta().parameterDef[srcPin.second].format)
    {
      createLink(col, uintpair(id, 0), srcPin);
    }
  }
}

} // namespace terra