
#pragma once

#include "Camera.h"
#include "GlGfx.h"
#include "Pipeline.h"
#include "Setup.h"
#include <optional>

namespace terra
{
class TerraMainApp;
class GfxDevice43;
class MeshPreview
{
public:
  MeshPreview();

  void init(TerraMainApp&);
  void deinit(TerraMainApp&);
  void regenerate(TerraMainApp const& app)
  {
    regenerate(app, actor);
  }
  void regenerate(TerraMainApp const&, HDataSource);

  void update(glm::ivec2 viewportSize, MouseState& ms)
  {
    updateSunDir(viewportSize, ms);
    camera.update(viewportSize, box, ms);
  }
  void draw(Rect const& viewport, Rect const& scissor, TerraMainApp&);

  void createDeviceObjects(TerraMainApp const&, GfxDevice43&);

  Property<Color> sunColor         = Property<Color>("@sunColor", 112, 82, 111, 255);
  Property<float> sunIntensity     = Property<float>("@sunIntensity", 0.4f);
  Property<Color> meshTint         = Property<Color>("@meshTint", 111, 111, 111, 255);
  Property<float> heightMultiplier = Property<float>("@heightMultiplier", 20.0f);
  // in this order: water,grass,rock,default
  Property<TextureFile> water               = Property<TextureFile>("@waterColor", "images/water_color.png");
  Property<TextureFile> vegetation          = Property<TextureFile>("@vegetationColor", "images/vegetation_color.png");
  Property<TextureFile> rocks               = Property<TextureFile>("@rockColor", "images/rocks_color.png");
  Property<TextureFile> terrain             = Property<TextureFile>("@rockColor", "images/terrain_color.png");
  Property<float>       meshStyle           = Property<float>("@meshStyle", 5.6f);
  Property<Rotation>    sunRotation         = Property<Rotation>("@sunRotation", 40.f);
  Property<uint32_t>    shadowMapResolution = Property<uint32_t>("@shadowMapResolution", 2);

  Camera& getCamera()
  {
    return camera;
  }

private:
  void updateShadowMap(TerraMainApp const&);
  void buildShadowMapProgram();
  void buildTerrainDrawProgram();
  void updateSunDir(glm::ivec2 viewportSize, MouseState& ms);
  void reloadTexture(TerraMainApp const&);

  float                         max         = 1.0f;
  float                         min         = -1.0f;
  uint32_t                      vertexCount = 0;
  glm::vec3                     box;
  Camera                        camera;
  std::shared_ptr<Pipeline>     pipeline;
  HDataSource                   actor;
  GfxBuffer::handle             index;
  GfxImage::handle              nullImage;
  GfxImage::handle              terrainColors;
  GfxImage::handle              shadowMapImage;
  GfxImage::handle              heights;
  GfxImage::handle              layerContrib;
  GfxCombinedImage::handle      shadowMap;
  GfxMesh::handle               layout;
  GfxSampler::handle            sampler;
  GfxSampler::handle            shadowSampler;
  ShaderProgram                 materialProg;
  ShaderProgram                 shadowProg;
  std::optional<ShaderMaterial> terrainMat;
  std::optional<ShaderMaterial> shadowMat;
  ShaderOptions                 shadowProgOptions;
  uvec2                         tileSize = {0, 0};
  GfxMesh::Draw                 drawCall;
  uint64_t                      setActorEventListener = 0;

  bool texturesDirty     = true;
  bool shadowMapResDirty = true;
  bool shadowMapDirty    = true;
  bool generated         = false;
};
} // namespace terra
