#pragma once
#include "Setup.h"
#include <memory>
#include <string>

#include <SDL.h>
#include "imgui.h"
#include "ImguiTheme.h"

namespace terra
{
class ImguiTerraContext;
class ImguiTerraWindow
{
public:
  void create(SDL_GLContext, AppSettings const&);
  bool pollEvents();
  void draw();
  void setTheme(ImguiTheme const&);

private:
  glm::ivec2 position = glm::ivec2(0, 0);
  glm::ivec2 size     = glm::ivec2(1024, 768);

  SDL_Window*   window       = nullptr;
  ImGuiContext* imguiContext = nullptr;
  uint32        windowID     = 0;
};
} // namespace terra