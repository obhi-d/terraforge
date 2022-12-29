
#include "Camera.h"
#include <glm/gtx/transform.hpp>

namespace terra
{

void Camera::update(glm::ivec2 viewportSize, glm::vec3 const& box, Rotation& sun, MouseState& ms)
{
  if (ms.locked != MouseLockedBy::eNone && ms.locked != MouseLockedBy::eCamera)
    return;

  if (ortho)
    distanceFactor = 2.f;

  auto newRadius = glm::length(box) * distanceFactor;
  if (newRadius != objRadius)
  {
    objRadius = radius = newRadius;
    view               = glm::translate(glm::mat4(1), glm::vec3(0, 0, -radius));
  }

  if (ms.mainWnd && ms.middleDelta != 0.0f)
  {
    radius -= ms.middleDelta * scrollSpeed;
  }

  if (ms.dragging && ms.mainWnd && ms.leftDown)
  {
    ms.locked                = MouseLockedBy::eCamera;
    float           x        = 0.5f * (float)ms.mouseDelta.x / (float)viewportSize.x;
    float           y        = -0.5f * (float)ms.mouseDelta.y / (float)viewportSize.y;
    constexpr float rotSpeed = 350.f;

    cameraRotation.thetaAdd(170.f * y);
    cameraRotation.phiAdd(350.f * x);
    sun.thetaAdd(170.f * y);
    sun.phiAdd(170.f * y);
  }

  view               = glm::lookAt(cameraRotation.toDir() * radius, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  this->viewportSize = viewportSize;

  // if (viewportSize != this->viewportSize)
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
      float max              = std::max(fabs(box.x), fabs(box.z)) * .5f;

      float r = max * imageAspectRatio, t = max;
      float l = -r, b = -t;
      projection = reverseZ ? glm::orthoRH_ZO(l, r, b, t, 9000.f, 0.f) : glm::orthoRH(l, r, b, t, 0.f, 9000.f);
    }
    else
    {
      float n = 0.01f;
      float f = 10000.f;
      projection =
        reverseZ ? perspectiveRH_RZ(glm::radians(fov.get()), (float)viewportSize.x / (float)viewportSize.y, n)
                 : glm::perspectiveFovRH(glm::radians(fov.get()), (float)viewportSize.x, (float)viewportSize.y, n, f);
    }
  }
}

void Camera::updateSunMatrix(glm::vec3 dir, float domeRad)
{
  const glm::mat4 biasMatrix =
    reverseZ ? glm::mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0)
             : glm::mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
  float halfD         = domeRad;
  mat4  lightView     = glm::lookAt(dir * halfD, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  mat4  lightProj     = reverseZ ? glm::orthoRH_ZO(-halfD, halfD, -halfD, halfD, 9000.f, 0.f)
                                 : glm::orthoRH(-halfD, halfD, -halfD, halfD, 0.f, 9000.f);
  mat4  biasLightProj = reverseZ ? orthoBiasRH_RZ(-halfD, halfD, -halfD, halfD, 0.f, 9000.f)
                                 : orthoBiasRH(-halfD, halfD, -halfD, halfD, 0.f, 9000.f);
  sunViewProj         = lightProj * lightView;
  biasSunViewProj     = biasLightProj * lightView;
}

} // namespace terra
