
#pragma once

#include "Camera.h"
#include "GlGfx.h"
#include "Pipeline.h"
#include "Setup.h"

namespace terra
{
class TerraMainApp;
class GfxDevice43;
class MeshPreview
{
public:
  void init(TerraMainApp&);
  void regenerate(TerraMainApp const&, dshandle);

  void update(glm::ivec2 viewportSize, MouseState& ms)
  {
    updateSunDir(viewportSize, ms);
    camera.update(viewportSize, box, ms);
  }
  void draw(glm::ivec2 viewportSize, TerraMainApp&);

  void createDeviceObjects(TerraMainApp const&, GfxDevice43&);

private:
  void updateSunDir(glm::ivec2 viewportSize, MouseState& ms);
  void reloadTexture(TerraMainApp const&);

  glm::vec4 sunColor = {0.45f, 0.64f, 0.22f, 1.1f};
  glm::vec4 meshTint = {0.15f, 0.14f, 0.12f, 1.f};

  Property<std::string> heightTexPath = "images/default_terrain.png";
  Property<float>       meshStyle     = 200.f;
  Property<Rotation>    sunRotation;

  float                     max         = 1.0f;
  float                     min         = -1.0f;
  uint32_t                  vertexCount = 0;
  glm::vec3                 box;
  Camera                    camera;
  LaunchParams              params;
  std::shared_ptr<Pipeline> pipeline;
  dshandle                  actor;
  GfxImage2D::handle        heightColors;
  GfxBuffer::handle         vertex;
  GfxBuffer::handle         index;
  GfxBuffer::handle         ubo;
  GfxMesh::handle           layout;
  GfxSampler::handle        sampler;
  GfxMaterial               material;
  glm::ivec2                tileSize       = {0, 0};
  glm::ivec2                nbPreviewTiles = {0, 0};
  GfxMesh::Draw             drawCall;
  bool                      descriptorsDirty = true;
  bool                      generated        = false;
};
} // namespace terra