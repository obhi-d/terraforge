
#pragma once
#include "Setup.h"
#include "Terra.h"
#include "ImguiTheme.h"
#include "imgui_node_editor.h"
#include "imgui/imgui.h"

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
  fOutput = 1 << 2,
  fInputPin = 1 << 3,
  fIsFilled    = 1 << 4,
};
class NodeEditor;
class DrawableNode
{
public:

  DrawableNode(TerraMainApp&, hnode id, ImVec2 pos);
  bool begin(TerraMainApp&, ImguiBackend&, NodeEditor&, bool& previewNode);
  void end(TerraMainApp&, ImguiBackend&, NodeEditor&);
  bool is(hnode id) const
  {
    return id == this->id;
  }

private:
 

  struct PinData
  {
    imne::PinId id;
    ImVec2        xy    = ImVec2(0,0);
    PinStateFlags flags = PinStateFlags::fNone;
  };

  void drawPinIcon(NodeEditor& , NodeStyle const&, PinData const&, DataFormat, bool detached);
  void drawHeader(NodeEditor&, NodeStyle const&, ImVec2 headerMin, ImVec2 headerMax);
  void drawParameter(NodeEditor&, NodeStyle const&, Node&, uint32_t);

  PinData output;
  std::vector<PinData> parameters;

  float    headerMaxY = 0.0f;

  hnode    id;
  uint32_t style;

  ImVec2   pos;
 
  bool     firstDraw = true;
};

struct Link
{
  imne::LinkId id;
  Color        color;
  imne::PinId start;
  imne::PinId end;
};

ENUM_FLAGS(PinStateFlags);
}