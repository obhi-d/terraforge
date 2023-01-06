#pragma once
#include "DrawableNode.h"
#include <acl/sparse_vector.hpp>

namespace terra
{
class ImguiBackend;
class TerraMainApp;

class NodeEditor
{
public:
  struct traits
  {
    using size_type                              = std::uint32_t;
    static constexpr std::uint32_t pool_size     = 32;
    static constexpr std::uint32_t idx_pool_size = 32;
    static constexpr bool          assume_pod_v  = false;
    // null
    // static constexpr T null_v = {};
    // using offset
    // using offset = acl::offset<&selfref::self>;
  };

  struct EventNodeCreate
  {
    NodeMeta const* meta = nullptr;
    ImVec2          pos;
  };

  void init(TerraMainApp& app);
  void deinit(TerraMainApp& app);
  bool drawNodeEditor(TerraMainApp&, ImguiBackend&);
  void drawNodeSettings(TerraMainApp&, ImguiBackend&);

  bool acceptsAction();

  void showTooltip(PinData pin);

  void showHelp(PinData pin)
  {
    pendingAction.action = Action::eShowHelp;
    pendingAction.pin    = pin;
  }

  void showTooltip(imne::NodeId pin)
  {
    pendingAction.action = Action::eShowTooltip;
    pendingAction.node   = pin;
  }

  void showHelp(imne::NodeId pin)
  {
    pendingAction.action = Action::eShowHelp;
    pendingAction.node   = pin;
  }

  void changeImage(HDataSource id);

  uint32_t getPreviewNodeStyle() const
  {
    return previewNodeStyle;
  }

  void tick();

private:
  void drawScalar(TerraMainApp& app, ImguiBackend& backend, ParameterMeta const&, Node&, uint32_t param);
  void drawParameter(TerraMainApp& app, ImguiBackend& backend, ParameterMeta const&, Node&, uint32_t param);

  void createLink(ImThemeColors const&, PinData start, PinData end);
  void deleteLink(imne::LinkId);
  void deleteNode(imne::NodeId);
  void doContextMenu(TerraMainApp&, ImVec2 pos);
  void doNodes(TerraMainApp&, ImguiBackend&);
  void createNode(TerraMainApp&, NodeMeta const& meta, ImVec2);
  void createCurveEditor(TerraMainApp&, ImVec2);
  void createImageNode(TerraMainApp&, std::filesystem::path, ImVec2);
  void executePendingAction(TerraMainApp&);
  void setNextDataSource(ImThemeColors const& col, HDataSource node, PinData src);
  void openImage(TerraMainApp& app);

  enum class Action
  {
    eCreateNode,
    eImportNode,
    eImageData,
    eChangeImage,
    eCurveData,
    ePasteNode,
    eShowTooltip,
    eShowHelp,
    eNone
  };

  struct ActionData
  {
    Action          action = Action::eNone;
    ImVec2          position;
    NodeMeta const* meta = nullptr;
    PinData         linkTo;
    PinData         pin;
    imne::NodeId    node;
  };

  struct FileOpen
  {
    Action      action;
    ImVec2      position;
    HDataSource node;
    PinData     linkTo;
  };

  FileOpen   fileOpenData;
  ActionData pendingAction;

  HDataSource previewNode;
  uint32_t    previewNodeVersion = 0;
  uint32_t    previewNodeStyle   = 0;

  using CategoryMap = std::vector<std::pair<std::u8string_view, std::vector<std::reference_wrapper<NodeMeta const>>>>;
  using NodeList    = std::vector<std::unique_ptr<DrawableNode>>;

  CategoryMap        cachedMetas;
  NodeList           drawableNodes;
  table<Link>        links;
  std::string        lastImagePath;
  std::u8string_view tipIncompatFormat;
  std::u8string_view tipIncompatType;
  std::u8string_view tipLink;
  std::u8string_view tipCreateNode;

  std::u8string_view actions;
  std::u8string_view dataNode;
  std::u8string_view importNode;
  std::u8string_view nodeEditor;
  std::u8string_view pasteNode;
  std::u8string_view toggleSelectedNode;

  // data nodes
  std::u8string_view curveNode;
  std::u8string_view imageNode;

  struct FrameCache
  {
    std::array<char, 64> filterData     = {};
    bool                 filterHasFocus = false;
    NodeMeta const*      createSelected = nullptr;
    PinData              linkTo;
  };

  FrameCache           frameCache;
  imne::EditorContext* editorContext     = nullptr;
  bool                 nodeRegenRequired = false;
  MenuData             window;
  MenuDelegate         openPreview;
  MenuDelegate         settingsWindow;
};

} // namespace terra
