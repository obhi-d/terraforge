#pragma once
#include "Setup.h"
#include <memory>
#include <string>

#include <SDL.h>
#include "imgui.h"
#include "ImguiTheme.h"
#include "ImguiBackend.h"
#include "NodeEditor.h"

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
  void drawTitlebar();
  void drawResizeControl();
  
private:
  enum WindowState
  {
    eMinimized,
    eMaximized,
    eWindowed
  };

  WindowState state    = WindowState::eWindowed;

  glm::ivec2 position = glm::ivec2(0, 0);
  glm::ivec2 size     = glm::ivec2(1024, 768);
  glm::ivec2  windowSize= glm::ivec2(1024, 768);
  glm::ivec2  mouseLatest = glm::ivec2(0, 0); 
  glm::ivec2  mouseLast = glm::ivec2(0, 0); 

  NodeEditor    nodeEditor;
  ImguiBackend  backend;
  SDL_Window*   window       = nullptr;
  ImGuiContext* imguiContext = nullptr;
  uint32        windowID     = 0;

  bool mouseDragging  = false;
  bool windowResizing = false;
  bool windowDragging = false;
};
} // namespace terra