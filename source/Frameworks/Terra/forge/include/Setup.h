#pragma once
#include "Common.h"
#include "ImageSerializer.h"
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
  Property<glm::uvec2> tileSize    = Property<glm::uvec2>("@tileSize", 257, 257);
  Property<glm::ivec2> tileOffset  = Property<glm::ivec2>("@tileOffset", 1, 1);
  Property<glm::uvec2> previewTile = Property<glm::uvec2>("@previewTile", 0, 0);
  // Property<glm::uvec2> nbPreviewTiles = Property<glm::ivec2>("@nbPreviewTiles", 1, 1);

  // Generation
  Property<float>    frequency = Property<float>("@frequency", 0.02f);
  Property<uint32_t> seed      = Property<uint32_t>("@seed", 1337);

  glm::uvec2 viewerSize = glm::uvec2(32, 32);
  glm::uvec2 viewerPos  = glm::uvec2(20, 20);

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

  bool load(ImageData&);

  bool reload(TerraMainApp const&);
};

enum class WindowAction
{
  eNone,
  eRestore,
  eMaximize,
  eClose
};

inline ImVec4 toImgui(glm::vec4 v)
{
  return ImVec4(v.x, v.y, v.z, v.w);
}

inline ImVec4 toImgui(Color v)
{
  return ImVec4(v.r(), v.g(), v.b(), v.a());
}

inline glm::mat4 reverseZRH_ZO(float fovY, float aspect, float zNear)
{
  float f = 1.0f / tan(fovY / 2.0f);
  return glm::mat4(f / aspect, 0.0f, 0.0f, 0.0f, 0.0f, f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, zNear, 0.0f);
}

} // namespace terra