
#include "Terra.h"
#include "Camera.h"
#include <glm/gtx/transform.hpp>

namespace terra
{

void Camera::update(glm::ivec2 viewportSize, glm::vec3 const& box, MouseState& ms)
{
  if (ms.locked != MouseLockedBy::eNone && ms.locked != MouseLockedBy::eCamera)
    return;

  if (ortho)
    distanceFactor = 2.f;

  auto newRadius = glm::length(box) * distanceFactor;
  if (newRadius != objRadius)
  {
    objRadius = radius = newRadius;
    view = glm::translate(glm::mat4(1), glm::vec3(0, 0, -radius));
  }
    
  if (ms.mainWnd && ms.middleDelta != 0.0f)
  {
    radius -= ms.middleDelta * scrollSpeed;
  }

  if (ms.dragging && ms.mainWnd && ms.leftDown)
  {
    ms.locked                = MouseLockedBy::eCamera;
    float           x  = 0.5f * (float)ms.mouseDelta.x / (float)viewportSize.x;
    float           y  =-0.5f * (float)ms.mouseDelta.y / (float)viewportSize.y;
    constexpr float rotSpeed = 350.f;

    cameraRotation.thetaAdd(170.f * y);
    cameraRotation.phiAdd(350.f * x);
  }

  view = glm::lookAt(cameraRotation.toDir() * radius, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  
  if (viewportSize != this->viewportSize)
  {
    if (ortho)
    {

      glm::vec4 minWorld = glm::vec4(box * -0.5f, 1.f);
      glm::vec4 maxWorld = glm::vec4(box * 0.5f, 1.f);
      auto      minCam   = view * minWorld;
      minCam             = minCam / minCam.w;
      auto maxCam        = view * maxWorld;
      maxCam             = maxCam / maxCam.w;

      float imageAspectRatio = (float)viewportSize.x / (float)viewportSize.y;
      float maxx             = std::max(fabs(minCam.x), fabs(maxCam.x));
      float maxy             = std::max(fabs(minCam.y), fabs(maxCam.y));
      
      float max              = std::max(maxx, maxy);
      float r = max * imageAspectRatio, t = max;
      float l = -r, b = -t;

      this->viewportSize = viewportSize;
      projection         = glm::ortho(l, r, b, t, -1.0f, 1000.f);
    }
    else
    {
      projection = glm::perspectiveFov(fov.get(), (float)viewportSize.x, (float)viewportSize.y, .01f, 10000.f);
    }
  }
}

}