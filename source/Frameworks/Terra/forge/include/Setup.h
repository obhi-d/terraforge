#pragma once
#include "Common.h"
#include "Logger.h"
#include "Property.h"
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

namespace terra
{
class TerraMainApp;
struct AppSettings
{
  std::string name;

  // Internal
  Property<std::string> language = Property<std::string>("@lang", "en-US");
  Property<std::string> theme    = Property<std::string>("@theme", "themes/default.tns");
  Property<bool>        verbose  = Property<bool>("@verbose", true);

  // Export
  Property<glm::ivec2> tileSize       = Property<glm::ivec2>("@tileSize", 257, 257);
  Property<glm::ivec2> tileOffset     = Property<glm::ivec2>("@tileOffset", 1, 1);
  Property<glm::ivec2> nbPreviewTiles = Property<glm::ivec2>("@nbPreviewTiles", 1, 1);

  // Generation
  Property<float> frequency = Property<float>("@frequency", 0.02f);
  Property<int>   seed      = Property<int>("@seed", 1337);

  glm::ivec2 viewerSize = glm::ivec2(32, 32);
  glm::ivec2 viewerPos  = glm::ivec2(20, 20);

  int  glslVersion     = 130;
  bool wasLoaded       = false;
  bool hasRobustAccess = true;
};

enum class MouseLockedBy
{
  eNone = 0,
  eMainWndDecorations,
  eSun,
  eCamera,
  eImgui
};

struct MouseState
{
  glm::ivec2    mousePosition;
  glm::ivec2    mouseDelta;
  float         middleDelta;
  MouseLockedBy locked    = MouseLockedBy::eNone;
  bool          leftDown  = false;
  bool          rightDown = false;
  bool          dragging  = false;
  bool          mainWnd   = false;
};

struct TextureFile
{
  TextureFile(std::string p) : path(p) {}
  std::string path;
  glm::ivec2  size{};
  uint32_t    image = 0;

  bool reload(TerraMainApp const&);
};

enum class WindowAction
{
  eNone,
  eRestore,
  eMaximize,
  eClose
};

} // namespace terra