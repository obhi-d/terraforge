
#include <filesystem>
#include "NodeEditor.h"
#include "ResourceUtils.h"
#include "ImguiBackend.h"
#include "imnodes.h"

namespace terra
{

void NodeEditor::scanNodeMetas() 
{
  for (auto const& dir_entry : std::filesystem::directory_iterator{getMediaPath() / "effects"})
  {
    if (dir_entry.is_regular_file() && dir_entry.path().has_extension() && dir_entry.path().extension() == ".ns")
      Terra::get().scanShader(dir_entry.path());
  }
}

void NodeEditor::drawNodeEditor(ImguiBackend& backend) 
{
  if (ImGui::Begin("Node Editor", nullptr))
  {
    auto newSize = ImGui::GetWindowSize();

    ImNodes::BeginNodeEditor();

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomLeft);

    ImNodes::EndNodeEditor();
  }
  ImGui::End();
}

}