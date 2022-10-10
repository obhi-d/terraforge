
#include <filesystem>
#include "NodeEditor.h"
#include "ResourceUtils.h"
#include "ImguiBackend.h"
#include "imgui_node_editor.h"
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
  
  importNode                         = app.getLocalizedString("@Editor.ImportNode");
  nodeEditor                         = app.getLocalizedString("@Editor.Name");
  pasteNode                          = app.getLocalizedString("@Editor.PasteNode");
  toggleSelectedNode                 = app.getLocalizedString("@Editor.ToggleSelectedNode");
  imne::Config config;
  config.SettingsFile = "terra-nodes.json";
  editorContext                      = imne::CreateEditor(&config);
}

void NodeEditor::drawNodeEditor(TerraMainApp& app, ImguiBackend& backend)
{
  imne::SetCurrentEditor(editorContext);
  if (ImGui::Begin((const char*)nodeEditor.data()))
  {
    imne::Begin((const char*)nodeEditor.data(), ImVec2(0, 0));
    {
      auto newSize = ImGui::GetWindowSize();
      imne::Suspend();
      if (imne::ShowBackgroundContextMenu())
        ImGui::OpenPopup("new_node");
      doContextMenu(app);
      imne::Resume();
      doNodes(app, backend);
      imne::End();
      imne::SetCurrentEditor(nullptr);
    }
  }
  ImGui::End();
}

void NodeEditor::doContextMenu(TerraMainApp& app) 
{
  enum class Action
  {
    eCreate,
    eImport,
    ePaste,
    eNone
  };

  
  Action todo     = Action::eNone;
  NodeMeta const* clicked  = nullptr;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
  ImVec2 pos;
  
  if (ImGui::BeginPopup("new_node"))
  {
    pos = ImGui::GetMousePosOnOpeningCurrentPopup();
    for (auto& c : cachedMetas)
    {
      if (!c.second.empty())
      {
        if (ImGui::BeginMenu((const char*)c.first.data()))
        {
          for (auto const& cr : c.second)
          {
            auto const& entry = cr.get();
            if (ImGui::MenuItemEx((const char*)entry.name.data(), (const char*)entry.icon.c_str()))
            {
              clicked = &entry;
              todo    = Action::eCreate;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
              ImGui::BeginTooltip();
              ImGui::TextUnformatted((const char*)entry.brief.data());
              ImGui::EndTooltip();
            }
          }
          ImGui::EndMenu();
        }
      }
    }
    
    if (ImGui::MenuItemEx((const char*)importNode.data(), ICON_FA_FILE_IMPORT))
    {
      todo = Action::eImport;
    }

    if (ImGui::MenuItemEx((const char*)importNode.data(), ICON_FA_PASTE))
    {
      todo = Action::ePaste;
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
  switch (todo)
  {
  case Action::eCreate:
    createNode(app, *clicked, pos);
    break;

  }
}

void NodeEditor::createNode(TerraMainApp& app, NodeMeta const& meta, ImVec2 pos)
{
  drawableNodes.emplace_back(app,
    get().createNode(meta), pos);
}

void NodeEditor::doNodes(TerraMainApp& app, ImguiBackend& backend)
{
  auto const& theme = app.getTheme();
  for (uint32_t n = 0; n < drawableNodes.size(); ++n)
  {
    auto&       dn    = drawableNodes[n];

    bool toggled = previewNode == n;
    if (dn.begin(app, backend, toggled))
    {
      previewNode          = n;
      nodeSelectionChanged = true;
    }
    dn.end(app, backend);
    /*
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::MenuItem(ICON_FA_OBJECT_GROUP " Node Tree"))
    }
    imne::EndNode();
    */
  }
}

}