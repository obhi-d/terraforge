#pragma once
#include "Common.h"
#include "Logger.h"
#include "Property.h"
#include <imgui.h>
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
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

struct Rotation
{
  // https://en.wikipedia.org/wiki/Spherical_coordinate_system
  float theta = 0.0f;   // 0 to 180
  float phi   = 180.0f; // 0 to 360

  Rotation() = default;
  Rotation(float the) : theta(the) {}

  void thetaAdd(float dt)
  {
    theta = std::clamp(theta + dt, 1.f, 179.f);
  }

  void phiAdd(float dt)
  {
    phi = std::clamp(phi + dt, 0.f, 360.f);
  }

  glm::vec3 toDir() const
  {
    auto theta    = glm::radians(this->theta);
    auto phi      = glm::radians(this->phi);
    auto sinTheta = std::sin(theta);
    auto cosTheta = std::cos(theta);
    auto sinPhi   = std::sin(phi);
    auto cosPhi   = std::cos(phi);
    return glm::normalize(glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi));
  }
};

struct TextureFile
{
  TextureFile(std::string p) : path(p) {}
  std::string path;
  glm::ivec2  size{};
  uint32_t    image = 0;

  bool reload(TerraMainApp const&);
};

// Color is ABGR, because of imgui
class Color
{
public:
  inline Color() = default;
  inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
  {
    color.r = r; // Extract the RR byte
    color.g = g; // Extract the GG byte
    color.b = b; // Extract the GG byte
    color.a = a; // Extract the BB byte
  }

  inline Color(uint32_t hexValue)
  {
    color.a = uint8_t((hexValue >> 24) & 0xFF); // Extract the RR byte
    color.b = uint8_t((hexValue >> 16) & 0xFF); // Extract the GG byte
    color.g = uint8_t((hexValue >> 8) & 0xFF);  // Extract the GG byte
    color.r = uint8_t((hexValue)&0xFF);         // Extract the BB byte
  }

  inline Color(ImVec4 f4)
  {
    color.r = (uint8_t)(f4.x * 255.f); // Extract the RR byte
    color.g = (uint8_t)(f4.y * 255.f); // Extract the GG byte
    color.b = (uint8_t)(f4.z * 255.f); // Extract the GG byte
    color.a = (uint8_t)(f4.w * 255.f); // Extract the BB byte
  }

  inline Color(glm::vec4 f4)
  {
    color.r = (uint8_t)(f4.x * 255.f); // Extract the RR byte
    color.g = (uint8_t)(f4.y * 255.f); // Extract the GG byte
    color.b = (uint8_t)(f4.z * 255.f); // Extract the GG byte
    color.a = (uint8_t)(f4.w * 255.f); // Extract the BB byte
  }

  inline operator uint32_t() const
  {
    return uint32_t{color.a} << 24 | uint32_t{color.b} << 16 | uint32_t{color.g} << 8 | uint32_t{color.r};
  }

  inline operator glm::vec4() const
  {
    return tovec4<glm::vec4>();
  }

  inline operator ImVec4() const
  {
    return tovec4<ImVec4>();
  }

  inline float r() const
  {
    return color.r / 255.f;
  }

  inline float g() const
  {
    return color.g / 255.f;
  }

  inline float b() const
  {
    return color.b / 255.f;
  }

  inline float a() const
  {
    return color.a / 255.f;
  }

private:
  template <typename T>
  T tovec4() const
  {
    return T{color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
  }

  glm::u8vec4 color = glm::u8vec4(255, 255, 255, 255);
};


struct Rect
{
  glm::ivec2 offset = glm::vec2(0, 0);
  glm::ivec2 size   = glm::vec2(1, 1);

  inline bool operator==(Rect const&) const noexcept   = default;
  inline bool operator!=(Rect const&) const noexcept = default;
};

enum class WindowAction
{
  eNone,
  eRestore,
  eMaximize,
  eClose
};

} // namespace terra