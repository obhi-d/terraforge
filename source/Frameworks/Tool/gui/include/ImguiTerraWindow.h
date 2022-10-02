#pragma once
#include <memory>
#include <string>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Timeline.h>

#include <SDL.h>

struct ImGuiContext;

using namespace Magnum;
namespace terra
{
class ImguiTerraContext;
class ImguiTerraWindow : public std::enable_shared_from_this<ImguiTerraWindow>
{
public:
  enum Type
  {
    eMainWindow,
    eFloating,
    eEmbedded
  };

  ImguiTerraWindow(ImguiTerraContext& context, std::string iname, Vector2i size, Vector2i pos, Type type);
  void resized(Vector2i size);
  void resizing(Vector2i size);
  void windowEvent(SDL_Event const&);
  void keyEvent(SDL_Event const&, bool pressed);
  void mouseButtonEvent(SDL_Event const&, bool pressed);
  void mouseScrollEvent(SDL_Event const&);
  void mouseMoveEvent(SDL_Event const&);

  uint32 getId() const
  {
    return windowID;
  }

private:
  Type               type = Type::eMainWindow;
  Magnum::Vector2i   position;
  Magnum::Vector2i   size;

  SDL_Window*        window       = nullptr;
  Magnum::Vector2    dpiScaling   = Magnum::Vector2(1.0f, 1.0f);
  ImGuiContext*      imguiContext = nullptr;
  ImguiTerraContext& imguiTerraContext;
  uint32             windowID = 0;
};
} // namespace terra