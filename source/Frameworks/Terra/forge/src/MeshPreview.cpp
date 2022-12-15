
#include "MeshPreview.h"
#include "ImageSerializer.h"
#include "ResourceUtils.h"
#include "SourceBuilder.h"
#include "TerraMainApp.h"

namespace tmpl
{
#include "glsl/mesh.glsl"
}

namespace terra
{

MeshPreview::MeshPreview()
{
  ShaderOptions::Dictionary dict;

  vegetation->path = (getMediaPath() / vegetation->path).string();
  water->path      = (getMediaPath() / water->path).string();
  rocks->path      = (getMediaPath() / rocks->path).string();
  terrain->path    = (getMediaPath() / terrain->path).string();

  dict.names.emplace_back("Enum_ShadowRes512");
  dict.names.emplace_back("Enum_ShadowRes1024");
  dict.names.emplace_back("Enum_ShadowRes2048");
  dict.names.emplace_back("Enum_ShadowRes4096");
  shadowProgOptions = ShaderOptions(ShaderOptions::addDictionary(std::move(dict)));
}

void MeshPreview::init(TerraMainApp& app)
{
  auto caps = app.getDevice()->getCaps();
  if (caps.ARB_clip_control != GlGfxSupport::eUnsupported)
    camera.setReverseZ();
  regenerate(app, actor);
  setActorEventListener = app.dispatcher().listen(
    [this](TerraMainApp& app, TerraMainApp::EventRegen const& ev)
    {
      regenerate(app, ev.actor ? ev.actor : actor);
    });
}

void MeshPreview::deinit(TerraMainApp& app)
{
  app.getDevice()->destroy(index);
  app.getDevice()->destroy(nullImage);
  app.getDevice()->destroy(terrainColors);
  app.getDevice()->destroy(shadowMapImage);
  app.getDevice()->destroy(heights);
  app.getDevice()->destroy(layerContrib);
  app.getDevice()->destroy(shadowMap);
  app.getDevice()->destroy(heightMap);
  app.getDevice()->destroy(layerColorMap);
  app.getDevice()->destroy(layerContribMap);
  app.getDevice()->destroy(layout);
  app.getDevice()->destroy(heightSampler);
  app.getDevice()->destroy(layerSampler);
  app.getDevice()->destroy(shadowSampler);

  app.dispatcher().remove(setActorEventListener);

  materialProg = {};
  shadowProg   = {};
}

void MeshPreview::regenerate(TerraMainApp const& app, HDataSource iactor)
{
  AppSettings const& settings = app.getSettings();

  if (settings.tileSize != tileSize || !index)
  {
    tileSize    = settings.tileSize;
    vertexCount = (tileSize.x) * (tileSize.y);
    box.x       = (float)(tileSize.x) + 1.f;
    box.y       = max - min + 1.f;
    box.z       = (float)(tileSize.y) + 1.f;
    if (index)
      app.getDevice()->destroy(index);
    auto     patchX    = (uint32_t)((tileSize.x) - 1);
    auto     patchY    = (uint32_t)((tileSize.y) - 1);
    uint32_t nbPatches = patchX * patchY * 6;

    const uint32_t smallIdx = (nbPatches < std::numeric_limits<uint16_t>::max()) ? 2 : 4;

    index =
      app.getDevice()->createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::fIndex, nbPatches * smallIdx);

    auto indices = (uint32_t*)app.getDevice()->mapBuffer(index, 0, nbPatches * smallIdx);
    for (uint32_t y = 0; y < patchY; ++y)
    {
      for (uint32_t x = 0; x < patchX; ++x)
      {
        auto patchId = uint32_t(y * patchX + x) * 6;
        auto vertId  = uint32_t(y * (patchX + 1) + x);

        indices[patchId + 0] = static_cast<uint32_t>(vertId);
        indices[patchId + 1] = static_cast<uint32_t>(vertId + patchX + 1);
        indices[patchId + 2] = static_cast<uint32_t>(vertId + 1);

        indices[patchId + 3] = static_cast<uint32_t>(vertId + 1);
        indices[patchId + 4] = static_cast<uint32_t>(vertId + patchX + 1);
        indices[patchId + 5] = static_cast<uint32_t>(vertId + patchX + 2);
      }
    }

    app.getDevice()->unmapBuffer(index);

    drawCall.indexBuffer.handle = index;
    drawCall.indexBufferStride  = smallIdx;
    drawCall.indexCount         = nbPatches;
    drawCall.layout             = {};
    drawCall.type               = GfxMesh::eTriangles;
  }

  actor = iactor;
  if (!pipeline)
  {
    pipeline = get().createPipeline();
  }

  if (actor)
  {
    pipeline->actor(actor);
    pipeline->frequency(settings.frequency);
    pipeline->offset(settings.tileOffset);
    pipeline->size(settings.tileSize);
    pipeline->seed(settings.seed);
    pipeline->compute(settings.previewTile);
  }
}

void MeshPreview::createDeviceObjects(TerraMainApp const& app, GfxDevice43& dev)
{
  GfxMesh::Layout mesh;
  mesh.vertexBufferCount = 0;
  layout                 = dev.createMeshLayout(mesh);
  buildShadowMapProgram();
  layerSampler  = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
  heightSampler = dev.createSampler(ImageSampling(SamplingType::eNearest, Tiling::eClampToEdge));
  shadowSampler = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge,
                                                  camera.isReverseZ() ? SampleCompare::eGEq : SampleCompare::eLEq));
}

void MeshPreview::reloadTexture(TerraMainApp const& app)
{
  std::vector<Color> layerColors;

  layerColors.resize(4 * 4, Color(0));
  if (!nullImage)
    nullImage = app.getDevice()->create2DImage(GfxStorageClass::eStaticDeviceReadonly, 4, 4, ImageFormatEnum::eRgba8,
                                               (ubyte_t const*)layerColors.data());

  static constexpr uint32_t Width = 256;
  layerColors.resize(Width * 4, Color(0));
  {
    Image image = Image(water->path);
    if (image.data && image.isRgba())
    {
      for (uint32_t i = 0; i < Width; ++i)
        layerColors[i] = image.bifilter_rgba((float)i / (float)Width, 0.f);
    }
  }
  {
    Image image = Image(vegetation->path);
    if (image.data && image.isRgba())
    {
      for (uint32_t i = 0; i < Width; ++i)
        layerColors[i + 256] = image.bifilter_rgba((float)i / (float)Width, 0.f);
    }
  }
  {
    Image image = Image(rocks->path);
    if (image.data && image.isRgba())
    {
      for (uint32_t i = 0; i < Width; ++i)
        layerColors[i + 512] = image.bifilter_rgba((float)i / (float)Width, 0.f);
    }
  }
  {
    Image image = Image(terrain->path);
    if (image.data && image.isRgba())
    {
      for (uint32_t i = 0; i < Width; ++i)
        layerColors[i + 768] = image.bifilter_rgba((float)i / (float)Width, 0.f);
    }
  }

  app.getDevice()->destroy(layerColorMap);
  app.getDevice()->destroy(terrainColors);

  terrainColors = app.getDevice()->create1DImageArray(GfxStorageClass::eStaticDeviceReadonly, Width, 4,
                                                      ImageFormatEnum::eRgba8, (ubyte_t const*)layerColors.data());
  layerColorMap = app.getDevice()->createCombinedTexture(terrainColors, layerSampler);
}

void MeshPreview::draw(Rect const& viewport, Rect const& scissor, TerraMainApp& app)
{
  GfxState state;
  if (texturesDirty)
  {
    reloadTexture(app);
    texturesDirty = false;
  }
  if (!generated)
  {
    createDeviceObjects(app, *app.getDevice());
    generated = true;
  }

  auto&            dev = *app.getDevice();
  GfxImage::handle lheight, llayer;
  pipeline->getResults(lheight, llayer);
  if (!lheight)
    lheight = nullImage;
  if (!llayer)
    llayer = nullImage;

  if (lheight != heights)
  {
    heights = lheight;
    dev.destroy(heightMap);
    heightMap = dev.createCombinedTexture(heights, heightSampler);
  }

  if (llayer != layerContrib)
  {
    layerContrib = llayer;
    dev.destroy(layerContribMap);
    layerContribMap = dev.createCombinedTexture(layerContrib, layerSampler);
  }

  state.blend[0].mode   = BlendMode::eDisabled;
  state.depthTest       = camera.isReverseZ() ? DepthTestMode::eGreaterEq : DepthTestMode::eLessEq;
  state.scissorsEnabled = true;
  state.viewport        = scissor;
  state.scissor         = scissor;

  AppSettings const& settings = app.getSettings();

  updateShadowMap(app);

  app.getDevice()->setState(state);
  app.getDevice()->clearBackbuffer(glm::vec4(app.getTheme().themeColors.clear),
                                   camera.isReverseZ() ? DepthClear::eClearZ_0 : DepthClear::eClearZ_1);

  drawTerrain(app);
}

void MeshPreview::updateSunDir(glm::ivec2 viewportSize, MouseState& ms)
{
  if (ms.locked != MouseLockedBy::eNone && ms.locked != MouseLockedBy::eSun)
    return;

  if (ms.dragging && ms.mainWnd && ms.rightDown)
  {
    ms.locked = MouseLockedBy::eSun;
    float x   = 0.5f * (float)ms.mouseDelta.x / (float)viewportSize.x;
    float y   = -0.5f * (float)ms.mouseDelta.y / (float)viewportSize.y;

    sunRotation->thetaAdd(170.f * y);
    sunRotation->phiAdd(350.f * x);
    shadowMapDirty = true;
  }
}

void MeshPreview::updateShadowMap(TerraMainApp const& app)
{
  if (!shadowMapDirty)
    return;

  auto save = shadowProgOptions;
  shadowProgOptions.setOption(shadowMapResolution);
  if (save != shadowProgOptions || !shadowMapImage)
  {
    uvec2 res = uvec2{512, 512};
    switch (shadowMapResolution.get())
    {
    case 1:
      res = uvec2{1024, 1024};
      break;
    case 2:
      res = uvec2{2048, 2048};
      break;
    case 3:
      res = uvec2{4096, 4096};
      break;
    }
    if (shadowMapImage)
      get().getDevice().destroy(shadowMapImage);
    shadowMapImage =
      get().getDevice().create2DImage(GfxStorageClass::eStaticDeviceReadonly, res.x, res.y, ImageFormatEnum::eDepth);
    buildTerrainDrawProgram();
  }

  if (!shadowMat)
    shadowMat.emplace(shadowProg);

  shadowMat->pushScalar(0, camera.getLightViewProj(sunRotation.get().toDir(), box));
  shadowMat->pushTexture(1, heightMap);
  shadowMat->pushScalar(2, tileSize.x);
  shadowMat->pushScalar(3, tileSize.y);
  shadowMat->pushScalar(4, 1.f / (float)tileSize.x);
  shadowMat->pushScalar(5, 1.f / (float)tileSize.y);
  shadowMat->pushScalar(6, heightMultiplier.get());
  shadowMat->pushOutput(7, shadowMapImage, true, vec4(camera.isReverseZ() ? 0.f : 1.f));
  app.getDevice()->draw(drawCall, shadowMat->program.material, shadowMat->data);

  shadowMapDirty = false;
}

void MeshPreview::buildShadowMapProgram()
{
  auto builder = app().getDevice()->createSourceBuilder(ShaderLang::eGLSL, SourceType::eShaderProgram);
  builder->sampleScalar("shadow_view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->sampleParam("heights",
                       DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eTexture));
  builder->sampleScalar("width", DataFormat(DataType::eUint, DataType::eUint));
  builder->sampleScalar("height", DataFormat(DataType::eUint, DataType::eUint));
  builder->sampleScalar("rwidth", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->sampleScalar("rheight", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->sampleScalar("height_multiplier", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->writeOutput(
    "depth", DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eDepth, ParamDeclType::eDepthOutput));

  auto code = fileContentToString("shaders/shadow.glsl");
  builder->append(code);
  shadowProg = builder->finalize();
}

void MeshPreview::drawTerrain(TerraMainApp const& app)
{
  if (!terrainMat)
    terrainMat.emplace(materialProg);

  terrainMat->pushScalar(0, camera.getLightViewProj(sunRotation.get().toDir(), box));
  terrainMat->pushScalar(1, camera.getViewProj());
  terrainMat->pushScalar(2, layerWeights.get());
  terrainMat->pushScalar(3, vec4(sunRotation.get().toDir(), sunIntensity.get()));
  terrainMat->pushTexture(4, heightMap);
  terrainMat->pushTexture(5, layerColorMap);
  terrainMat->pushTexture(6, shadowMap);
  terrainMat->pushTexture(7, layerContribMap);
  terrainMat->pushScalar(8, tileSize.x);
  terrainMat->pushScalar(9, tileSize.y);
  terrainMat->pushScalar(10, 1.f / (float)tileSize.x);
  terrainMat->pushScalar(11, 1.f / (float)tileSize.y);
  terrainMat->pushScalar(12, heightMultiplier.get());
  app.getDevice()->draw(drawCall, terrainMat->program.material, terrainMat->data);
}

void MeshPreview::buildTerrainDrawProgram()
{
  auto builder = app().getDevice()->createSourceBuilder(ShaderLang::eGLSL, SourceType::eShaderProgram);
  shadowProgOptions.setOption((uint32_t)shadowMapResolution.get());

  builder->pushOptions(shadowProgOptions);
  builder->sampleScalar("shadow_view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->sampleScalar("view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->sampleScalar("layer_weights", DataFormat(DataType::eFloat4, DataType::eFloat4));
  builder->sampleScalar("sun_data", DataFormat(DataType::eFloat4, DataType::eFloat4));
  builder->sampleParam("heights",
                       DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eTexture));
  builder->sampleParam("layer_colors",
                       DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eRgba32f, ParamDeclType::eTexture));
  builder->sampleParam("shadow_map",
                       DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eTexture));
  builder->sampleParam("layers",
                       DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eRgba32f, ParamDeclType::eTexture));
  builder->sampleScalar("width", DataFormat(DataType::eUint, DataType::eUint));
  builder->sampleScalar("height", DataFormat(DataType::eUint, DataType::eUint));
  builder->sampleScalar("rwidth", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->sampleScalar("rheight", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->sampleScalar("height_multiplier", DataFormat(DataType::eFloat, DataType::eFloat));
  auto code = fileContentToString("shaders/terrain.glsl");
  builder->append(code);
  materialProg = builder->finalize();
}

} // namespace terra
