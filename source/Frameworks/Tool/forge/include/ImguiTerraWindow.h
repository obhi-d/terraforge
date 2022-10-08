#pragma once
#include "Setup.h"
#include <memory>
#include <string>

#include "ImguiBackend.h"
#include "ImguiTheme.h"
#include "NodeEditor.h"
#include "imgui.h"
#include <SDL.h>

namespace terra
{
class GfxDevice43;
class ImguiTerraContext;
class ImguiTerraWindow
{
public:
  void create(SDL_GLContext, std::shared_ptr<GfxDevice43> device, AppSettings const&);
  bool pollEvents();
  void draw();
  void setTheme(ImguiTheme const&);
  void drawWindowDecoration();

private:
  struct DragData
  {
    glm::ivec2 startPosition;
    glm::ivec2 startSize;
    glm::ivec2 mouse = glm::ivec2(0, 0);
  };

  NodeEditor    nodeEditor;
  ImguiBackend  backend;
  SDL_Window*   window       = nullptr;
  ImGuiContext* imguiContext = nullptr;
  uint32        windowID     = 0;

  DragData dragData;
  bool mouseDragging  = false;
  bool windowResizing = false;
  bool windowDragging = false;
};
} // namespace terra