
#pragma once
#include "Terra.h"
#include "imgui_node_editor.h"
#include "imgui/imgui.h"

namespace imne = ax::NodeEditor;

namespace terra
{
class ImguiBackend;
class TerraMainApp;
struct NodeStyle;
class DrawableNode
{
public:
  DrawableNode(TerraMainApp&, hnode id, ImVec2 pos);
  bool begin(TerraMainApp&, ImguiBackend&, bool& previewNode);
  void end(TerraMainApp&, ImguiBackend&);

private:

  void drawPinIcon(NodeStyle const&, DataFormat, bool detached);

  float width() const
  {
    return max.x - min.x;
  }

  float height() const
  {
    return max.y - min.y;
  }

  imne::PinId output;

  hnode    id;
  uint32_t style;
  ImVec2   min       = ImVec2(0, 0);
  ImVec2   max       = ImVec2(6, 6);
  bool     firstDraw = true;
};

}