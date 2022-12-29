
#pragma once

#include "Setup.h"

namespace terra
{

class Camera
{
public:
  void setReverseZ()
  {
    reverseZ = true;
  }

  bool isReverseZ() const
  {
    return reverseZ;
  }

  void update(glm::ivec2 viewportSize, glm::vec3 const& box, Rotation& sun, MouseState&);
  void updateSunMatrix(glm::vec3 dir, float domeRad);

  mat4 const& getLightViewProj() const
  {
    return sunViewProj;
  }

  mat4 const& getLightViewProjBias() const
  {
    return biasSunViewProj;
  }

  glm::mat4 getViewProj() const
  {
    return projection * view;
  }

  Property<bool>  ortho          = Property<bool>("@ortho", false);
  Property<float> fov            = Property<float>("@fov", 45.f);
  Property<float> distanceFactor = Property<float>("@distanceFactor", 1.f);
  Property<float> scrollSpeed    = Property<float>("@scrollSpeed", 25.f);

private:
  float    objRadius      = 0.0f;
  float    radius         = 20.0f;
  Rotation cameraRotation = Rotation(80.f);

  glm::ivec2 viewportSize = {0, 0};

  glm::mat4 view = glm::mat4(1);
  glm::mat4 projection;
  glm::mat4 sunViewProj;
  glm::mat4 biasSunViewProj;

  bool reverseZ = false;
};

} // namespace terra