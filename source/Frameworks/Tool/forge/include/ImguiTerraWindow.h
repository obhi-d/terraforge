#pragma once
#include "Setup.h"
#include <memory>
#include <string>

#include "ImguiBackend.h"
#include "ImguiTheme.h"
#include "MeshPreview.h"
#include "NodeEditor.h"
#include "imgui.h"
#include <SDL.h>

namespace terra
{
class GfxDevice43;
class ImguiTerraContext;
class TerraMainApp;
class ImguiTerraWindow
{
public:
  void drawSettings(TerraMainApp&);
  void init(TerraMainApp&);
  bool pollEvents();
  void draw(TerraMainApp&);
  void setTheme(ImguiTheme const&);
  void drawWindowDecoration();

  inline MouseState const& getMouseState() const
  {
    return mouseState;
  }

  inline MouseState& getMouseState()
  {
    return mouseState;
  }

private:

  void setActor(TerraMainApp& app, dshandle actor)
  {
    meshPreview.regenerate(app, actor);
  }

  struct DragData
  {
    glm::ivec2 startPosition;
    glm::ivec2 startSize;
    glm::ivec2 mouse = glm::ivec2(0, 0);
  };

  std::u8string_view settingName;

  MeshPreview   meshPreview;
  NodeEditor    nodeEditor;
  ImguiBackend  backend;
  SDL_Window*   window       = nullptr;
  ImGuiContext* imguiContext = nullptr;
  uint32        windowID     = 0;

  glm::ivec2 windowSize;
  DragData   dragData;
  MouseState mouseState;
  bool       windowResizing = false;
  bool       windowDragging = false;
};
} // namespace terra
