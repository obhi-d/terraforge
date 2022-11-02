#pragma once
#include "Common.h"
#include "Logger.h"
#include "Property.h"
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace terra
{
struct AppSettings
{
  std::string           name;
  Property<std::string> language       = "en-US";
  Property<glm::ivec2>  viewerSize     = glm::ivec2(1024, 768);
  Property<glm::ivec2>  viewerPos      = glm::ivec2(20, 20);
  Property<glm::ivec2>  tileSize       = {121, 121};
  Property<glm::ivec2>  tileOffset     = {1, 1};
  Property<glm::ivec2>  nbPreviewTiles = {1, 1};

  Property<std::string> theme            = "themes/default.tns";
  Property<float>       frequency        = 0.1f;
  Property<float>       heightMultiplier = 1.0f;
  Property<int>         seed             = 1337;
  Property<int>         glslVersion      = 130;
  Property<bool>        verbose          = true;
  bool                  wasLoaded        = false;
  bool                  hasRobustAccess  = true;
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

} // namespace terra