
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
  void deinit(TerraMainApp&);
  void regenerate(TerraMainApp const& app)
  {
    regenerate(app, actor);
  }
  void regenerate(TerraMainApp const&, dshandle);

  void update(glm::ivec2 viewportSize, MouseState& ms)
  {
    updateSunDir(viewportSize, ms);
    camera.update(viewportSize, box, ms);
  }
  void draw(glm::ivec2 viewportSize, TerraMainApp&);

  void createDeviceObjects(TerraMainApp const&, GfxDevice43&);


  Property<Color>       sunColor         = Property<Color>("@sunColor", 112, 82, 111, 255);
  Property<float>       sunIntensity     = Property<float>("@sunIntensity", 0.4f);
  Property<Color>       meshTint         = Property<Color>("@meshTint", 111, 111, 111, 255);
  Property<float>       heightMultiplier = Property<float>("@heightMultiplier", 1.0f);
  Property<TextureFile> heightTexPath    = Property<TextureFile>("@heightTexture", "images/default_terrain.png");
  Property<float>       meshStyle        = Property<float>("@meshStyle", 200.f);
  Property<Rotation>    sunRotation      = Property<Rotation>("@sunRotation", 40.f);

private:
  void updateSunDir(glm::ivec2 viewportSize, MouseState& ms);
  void reloadTexture(TerraMainApp const&);

  

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
  uint64_t                  setActorEventListener = 0;
  bool                      descriptorsDirty = true;
  bool                      generated        = false;
};
} // namespace terra
