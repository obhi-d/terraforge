
#pragma once
#include "ImguiTheme.h"
#include "Setup.h"
#include "Terra.h"
#include "imgui.h"
#include "imgui_node_editor.h"

namespace imne = ax::NodeEditor;

namespace terra
{

class ImguiBackend;
class TerraMainApp;

struct NodeStyle;
// enum class PinStateFlags
//{
//   fNone        = 0,
//   fShowTooltip = 1 << 0,
//   fShowHelp    = 1 << 1,
//   fOutput      = 1 << 2,
//   fInputPin    = 1 << 3,
//   fIsFilled    = 1 << 4,
// };
class NodeEditor;
class DrawableNode
{
public:
  inline static constexpr uint32_t IsExecuting = 1;
  inline static constexpr uint32_t IsSelected  = 2;

  DrawableNode(TerraMainApp&, HDataSource id, ImVec2 pos);
  ~DrawableNode();
  bool begin(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t styleFlags);
  void end(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t styleFlags);
  bool is(HDataSource id) const
  {
    return id == this->id;
  }

  HDataSource getId() const
  {
    return id;
  }

  void updateThumbnailFromImage(Image&);

private:
  using PinData = imne::PinId;

  void drawPinIcon(NodeEditor&, NodeStyle const&, imne::PinId id, const char* name, DataFormat type, bool output,
                   bool detached);
  void drawHeader(NodeEditor&, NodeStyle const&, ImVec2 headerMin, ImVec2 headerMax);
  void drawParameter(NodeEditor&, NodeStyle const&, Node&, uint32_t);

  std::vector<PinData> parameters;
  std::vector<PinData> outputs;

  float headerMaxX = 0.0f;

  HDataSource id;
  uint32_t    style = 0;

  ImVec2 pos{};

  static inline float constexpr ThumbnailSize = 200.f;
  GfxImage::handle thumbnail;
  uint32_t         thumbnailVersion = std::numeric_limits<uint32_t>::max();

  bool firstDraw = true;
};

struct Link
{
  imne::LinkId id;
  Color        color;
  imne::PinId  start;
  imne::PinId  end;
};
} // namespace terra