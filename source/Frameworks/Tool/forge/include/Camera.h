
#pragma once

#include "Setup.h"

namespace terra
{

class Camera
{
public:
  void update(glm::ivec2 viewportSize, glm::vec3 const& box, MouseState&);

  glm::mat4 getViewProj() const
  {
    return projection * view;
  }

private:
  Property<bool>  ortho          = Property<bool>("@ortho", false);
  Property<float> fov            = Property<float>("@fov", 45.f);
  Property<float> distanceFactor = Property<float>("@distanceFactor", 1.f );
  Property<float> scrollSpeed    = Property<float>("@scrollSpeed", 1.f );

  float    objRadius      = 0.0f;
  float    radius         = 20.0f;
  Rotation cameraRotation = Rotation(80.f);

  glm::ivec2 viewportSize = {0, 0};

  glm::mat4 view = glm::mat4(1);
  glm::mat4 projection;
};

} // namespace terra