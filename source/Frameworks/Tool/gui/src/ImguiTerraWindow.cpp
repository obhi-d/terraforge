
#include <Common.h>
#include <ImguiTerraWindow.h>
#include <imgui.h>

namespace terra
{

ImguiTerraWindow::ImguiTerraWindow(ImguiTerraContext& context, std::string name, Vector2i size, Vector2i pos, Type type)
    : imguiTerraContext(context)
{
  this->size     = size;
  this->position = pos;
  this->type     = type;

  if (type != Type::eEmbedded)
  {
    auto scaledWindowSize = dpiScaling * size;
    window                = SDL_CreateWindow(name.data(), pos.x(), pos.y(), scaledWindowSize.x(), scaledWindowSize.y(),
                                             SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);

    if (window)
      windowID = SDL_GetWindowID(window);
    imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imguiContext);
  }
}

void ImguiTerraWindow::resized(Vector2i size) {}

void ImguiTerraWindow::resizing(Vector2i size) {}

void ImguiTerraWindow::windowEvent(SDL_Event const&) {}

void ImguiTerraWindow::keyEvent(SDL_Event const&, bool pressed) {}

void ImguiTerraWindow::mouseButtonEvent(SDL_Event const&, bool pressed) {}

void ImguiTerraWindow::mouseScrollEvent(SDL_Event const&) {}

void ImguiTerraWindow::mouseMoveEvent(SDL_Event const&) {}

} // namespace terra