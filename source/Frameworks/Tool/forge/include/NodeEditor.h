#pragma once
#include "DrawableNode.h"

namespace terra
{
class ImguiBackend;
class TerraMainApp;

class NodeEditor
{
public:

  struct EventNodeCreate
  {
    NodeMeta const* meta = nullptr;
    ImVec2 pos;
  };

  void init(TerraMainApp& app);
  void deinit(TerraMainApp& app);
  void drawNodeEditor(TerraMainApp&, ImguiBackend&);

  bool acceptsAction();

  void showTooltip(imne::PinId pin);

  void showHelp(imne::PinId pin) 
  {
    pendingAction.action = Action::eShowHelp;
    pendingAction.pin    = pin;
  }

  void showTooltip(imne::NodeId pin)
  {
    pendingAction.action = Action::eShowTooltip;
    pendingAction.node  = pin;
  }

  void showHelp(imne::NodeId pin)
  {
    pendingAction.action = Action::eShowHelp;
    pendingAction.node   = pin;
  }

private:

  void createLink(ImThemeColors const&, uintpair start, uintpair end);
  void deleteLink(imne::LinkId);
  void deleteNode(imne::NodeId);
  void doContextMenu(TerraMainApp&, ImVec2 pos);
  void doNodes(TerraMainApp&, ImguiBackend&);
  void createNode(TerraMainApp&, NodeMeta const& meta, ImVec2);
  void executePendingAction(TerraMainApp&);
  void setNextDataSource(ImThemeColors const& col, dshandle node, imne::PinId src);

  enum class Action
  {
    eCreateNode,
    eImportNode,
    ePasteNode,
    eShowTooltip,
    eShowHelp,
    eNone
  };

  struct ActionData
  {
    Action action = Action::eNone;
    ImVec2          position;
    NodeMeta const*    meta = nullptr;
    imne::PinId     linkTo; 
    imne::PinId pin;
    imne::NodeId node;
  };

  ActionData pendingAction;

  uint32_t previewNode = 0;

  using CategoryMap = std::vector<std::pair<std::u8string_view, std::vector<std::reference_wrapper<NodeMeta const>>>>;
  CategoryMap               cachedMetas;
  std::vector<DrawableNode> drawableNodes;
  table<Link>         links;

  std::u8string_view tipIncompatFormat;
  std::u8string_view tipIncompatType;
  std::u8string_view tipLink;
  std::u8string_view tipCreateNode;

  std::u8string_view        actions;
  std::u8string_view        importNode;
  std::u8string_view        nodeEditor;
  std::u8string_view        pasteNode;
  std::u8string_view        toggleSelectedNode;

  struct FrameCache
  {
    std::array<char, 64> filterData = {};
    bool                 filterHasFocus = false;
    NodeMeta const*      createSelected = nullptr;
    imne::PinId          linkTo;
  };

  FrameCache           frameCache;
  bool                 nodeSelectionChanged = false;
  imne::EditorContext* editorContext        = nullptr;

};

} // namespace terra
