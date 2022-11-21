
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
enum class PinStateFlags
{
  fNone        = 0,
  fShowTooltip = 1 << 0,
  fShowHelp    = 1 << 1,
  fOutput      = 1 << 2,
  fInputPin    = 1 << 3,
  fIsFilled    = 1 << 4,
};
class NodeEditor;
class DrawableNode
{
public:
  DrawableNode(TerraMainApp&, dshandle id, ImVec2 pos);
  ~DrawableNode();
  bool begin(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t selectedStyle);
  void end(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t selectedStyle);
  bool is(dshandle id) const
  {
    return id == this->id;
  }

  dshandle getId() const
  {
    return id;
  }

  void updateThumbnailFromImage(Image&);

private:
  struct PinData
  {
    imne::PinId   id;
    ImVec2        xy    = ImVec2(0, 0);
    PinStateFlags flags = PinStateFlags::fNone;
  };

  void drawPinIcon(NodeEditor&, NodeStyle const&, PinData const&, DataFormat, bool detached);
  void drawHeader(NodeEditor&, NodeStyle const&, ImVec2 headerMin, ImVec2 headerMax);
  void drawParameter(NodeEditor&, NodeStyle const&, Node&, uint32_t);

  PinData              output;
  std::vector<PinData> parameters;

  float headerMaxY = 0.0f;

  dshandle id;
  uint32_t style = 0;

  ImVec2 pos{};

  static inline float constexpr ThumbnailSize = 200.f;
  GfxImage2D::handle thumbnail;
  uint32_t           thumbnailVersion = std::numeric_limits<uint32_t>::max();

  bool firstDraw = true;
};

struct Link
{
  imne::LinkId id;
  Color        color;
  imne::PinId  start;
  imne::PinId  end;
};

ENUM_FLAGS(PinStateFlags);
} // namespace terra