#pragma once
#include "DrawableNode.h"

namespace terra
{
class ImguiBackend;
class TerraMainApp;

class NodeEditor
{
public:
  void init(TerraMainApp& app);
  void drawNodeEditor(TerraMainApp&, ImguiBackend&);
  void doContextMenu(TerraMainApp&, ImVec2 pos);
  void doNodes(TerraMainApp&, ImguiBackend&);
  void createNode(TerraMainApp&, NodeMeta const& meta, ImVec2);

private:
 
  uint32_t previewNode = 0;

  using CategoryMap = std::vector<std::pair<std::u8string_view, std::vector<std::reference_wrapper<NodeMeta const>>>>;
  CategoryMap               cachedMetas;
  std::vector<DrawableNode> drawableNodes;
  std::u8string_view        importNode;
  std::u8string_view        nodeEditor;
  std::u8string_view        pasteNode;
  std::u8string_view        toggleSelectedNode;

  bool                 nodeSelectionChanged = false;
  imne::EditorContext* editorContext        = nullptr;
};

} // namespace terra