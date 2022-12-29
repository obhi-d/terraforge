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
  static inline constexpr uint32_t titlebarHeight = 60;

  void init(TerraMainApp&);
  void deinit(TerraMainApp&);

  void drawSettings(TerraMainApp&);
  bool pollEvents();
  bool draw(TerraMainApp&);
  void tick();
  void setTheme(ImguiTheme const&);

  inline MouseState const& getMouseState() const
  {
    return mouseState;
  }

  inline MouseState& getMouseState()
  {
    return mouseState;
  }

  inline bool isPreviewOpen() const
  {
    return previewWindow.opened;
  }

  inline bool isSettingsOpen() const
  {
    return settingsWindow.opened;
  }

  inline void openPreview()
  {
    previewWindow.opened = true;
  }

  inline void openSettings()
  {
    settingsWindow.opened = true;
  }

private:
  void setActor(TerraMainApp& app, HDataSource actor)
  {
    meshPreview.regenerate(app, actor);
  }

  struct DragData
  {
    glm::ivec2 startPosition;
    glm::ivec2 startSize;
    glm::ivec2 mouse = glm::ivec2(0, 0);
  };

  ImguiBackend::CallbackData meshDrawData;

  std::u8string_view mainWindowName;
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

  MenuData previewWindow;
  MenuData settingsWindow;
};
} // namespace terra
