
#pragma once

#include "Camera.h"
#include "Canvas.h"
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

  void update(glm::ivec2 viewportSize, MouseState& ms);
  void draw(Rect const& viewport, Rect const& scissor, TerraMainApp&);

  void createDeviceObjects(TerraMainApp const&, GfxDevice43&);

  Property<Color> sunColor     = Property<Color>("@sunColor", (uint8_t)112, 82, 111, 255);
  Property<float> sunIntensity = Property<float>("@sunIntensity", 50.0f);
  Property<Color> meshTint     = Property<Color>("@meshTint", (uint8_t)111, 111, 111, 255);
  // in this order: water,grass,rock,default
  Property<TextureFile> water               = Property<TextureFile>("@waterColor", "images/water_color.png");
  Property<TextureFile> vegetation          = Property<TextureFile>("@vegetationColor", "images/vegetation_color.png");
  Property<TextureFile> rocks               = Property<TextureFile>("@rockColor", "images/rocks_color.png");
  Property<TextureFile> terrain             = Property<TextureFile>("@rockColor", "images/terrain_color.png");
  Property<float>       planetScale         = Property<float>("@planetScale", 1.0f);
  Property<float>       heightScale         = Property<float>("@heightScale", 10.0f);
  Property<Rotation>    sunRotation         = Property<Rotation>("@sunRotation", 60.f, 80.f);
  Property<uint32_t>    shadowMapResolution = Property<uint32_t>("@shadowMapResolution", 2);
  Property<vec4>        layerWeights        = Property<vec4>("@layerWeights", vec4(0.25f));
  Property<bool>        showWaterLevel      = Property<bool>("@showWaterLevel", true);
  Property<float>       wavePeriod          = Property<float>("@wavePeriodicty", 3.0f);
  Property<float>       waveLevel           = Property<float>("@waveLevel", -0.5f);

  Camera& getCamera()
  {
    return camera;
  }

  void tick();

private:
  void updateShaders(TerraMainApp const&);
  void updateShadowMap(TerraMainApp const&);
  void drawTerrain(TerraMainApp const&);
  // void drawWater(TerraMainApp const&);
  void drawAtmosphere(TerraMainApp const&);
  void buildShadowMapProgram();
  // void buildWaterProgram();
  void buildTerrainDrawProgram();
  void buildScatterProgram();
  void updateSunDir(glm::ivec2 viewportSize, MouseState& ms);
  void reloadTexture(TerraMainApp const&);

  float                     max         = 1.0f;
  float                     min         = -1.0f;
  float                     domeRadius  = 0.f;
  uint32_t                  vertexCount = 0;
  glm::vec3                 box;
  Camera                    camera;
  std::shared_ptr<Pipeline> pipeline;
  HDataSource               actor;
  GfxBuffer::handle         index;
  GfxImage::handle          nullImage;
  GfxImage::handle          terrainColors;
  GfxImage::handle          oceanNormalFoam;
  GfxImage::handle          shadowMapImage;
  GfxImage::handle          heights;
  GfxImage::handle          waterContrib;
  GfxImage::handle          vegetationContrib;
  GfxImage::handle          rocksContrib;
  GfxMesh::handle           layout;
  GfxSampler::handle        layerSampler;
  GfxSampler::handle        nearestSampler;
  GfxSampler::handle        shadowSampler;
  GfxSampler::handle        repeatSampler;
  GfxPass::handle           shadowGen;
  ShaderProgramPtr          materialProg;
  ShaderProgramPtr          shadowProg;
  ShaderProgramPtr          atmosphereProg;
  // ShaderProgramPtr              waterProg;
  std::optional<ShaderMaterial> terrainMat;
  std::optional<ShaderMaterial> shadowMat;
  std::optional<ShaderMaterial> atmosphereMat;
  // std::optional<ShaderMaterial> waterMat;
  ShaderOptions shadowProgOptions;
  ShaderOptions terrainProgOptions;
  uvec2         tileSize     = {0, 0};
  uvec2         shadowMapRez = {512, 512};
  GfxMesh::Draw drawCall;
  // GfxMesh::Draw waterDrawCall;
  uint64_t setActorEventListener = 0;
  Canvas   canvas;

  bool texturesDirty     = true;
  bool shadowMapResDirty = true;
  bool shadowMapDirty    = true;
  bool generated         = false;
};
} // namespace terra
