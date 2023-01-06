
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
  auto const& sett = get().getSettings();
  if (sett.reverseZ)
    camera.setReverseZ();
  regenerate(app, actor);
  setActorEventListener = app.dispatcher().listen(
    [this](TerraMainApp& app, TerraMainApp::EventRegen const& ev)
    {
      regenerate(app, ev.actor ? ev.actor : actor);
    });
  canvas.color(ImageFormatEnum::eRgba8);
  canvas.depth(camera.isReverseZ() ? ImageFormatEnum::eDepth32f : ImageFormatEnum::eDepth24);
}

void MeshPreview::deinit(TerraMainApp& app)
{
  app.getDevice()->destroy(index);
  app.getDevice()->destroy(nullImage);
  app.getDevice()->destroy(terrainColors);
  app.getDevice()->destroy(shadowMapImage);
  app.getDevice()->destroy(layout);
  app.getDevice()->destroy(layerSampler);
  app.getDevice()->destroy(shadowSampler);
  app.getDevice()->destroy(shadowGen);
  // app.getDevice()->destroy(heights);
  // app.getDevice()->destroy(layerContrib);

  app.dispatcher().remove(setActorEventListener);

  materialProg   = {};
  shadowProg     = {};
  atmosphereProg = {};
  pipeline       = {};
  terrainMat.reset();
  shadowMat.reset();
  atmosphereMat.reset();
  canvas.clear();
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
    domeRadius  = glm::length(box);

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
  drawCall.layout        = layout;
  buildShadowMapProgram();
  buildScatterProgram();
  layerSampler  = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
  shadowSampler = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge,
                                                  camera.isReverseZ() ? SampleCompare::eGT : SampleCompare::eLT));
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

  app.getDevice()->destroy(terrainColors);
  terrainColors = app.getDevice()->create1DImageArray(GfxStorageClass::eStaticDeviceReadonly, Width, 4,
                                                      ImageFormatEnum::eRgba8, (ubyte_t const*)layerColors.data());
}

void MeshPreview::draw(Rect const& viewport, Rect const& scissor, TerraMainApp& app)
{
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
  if (!drawCall.indexCount)
    return;

  auto&            dev = *app.getDevice();
  GfxImage::handle lheight;
  Pipeline::Layers llayer;
  pipeline->getResults(lheight, llayer);
  heights           = lheight ? lheight : nullImage;
  waterContrib      = llayer.water ? llayer.water : nullImage;
  rocksContrib      = llayer.rocks ? llayer.rocks : nullImage;
  vegetationContrib = llayer.vegetation ? llayer.vegetation : nullImage;

  AppSettings const& settings = app.getSettings();

  updateShadowMap(app);

  canvas.resize(scissor.size);
  canvas.begin(camera.isReverseZ());

  drawAtmosphere(app);
  drawTerrain(app);

  canvas.end();

  Rect src;
  src.size = canvas.getSize();
  app.getDevice()->blit(canvas.get(0), {}, src, scissor);
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
  // if (!shadowMapDirty)
  //   return;
  //
  auto save = shadowProgOptions;
  shadowProgOptions.setOption(shadowMapResolution);
  if (save != shadowProgOptions || !shadowMapImage)
  {
    switch (shadowMapResolution.get())
    {
    case 1:
      shadowMapRez = uvec2{1024, 1024};
      break;
    case 2:
      shadowMapRez = uvec2{2048, 2048};
      break;
    case 3:
      shadowMapRez = uvec2{4096, 4096};
      break;
    }
    if (shadowMapImage)
      get().getDevice().destroy(shadowMapImage);
    shadowMapImage =
      get().getDevice().create2DImage(GfxStorageClass::eStaticDeviceReadonly, shadowMapRez.x, shadowMapRez.y,
                                      camera.isReverseZ() ? ImageFormatEnum::eDepth32f : ImageFormatEnum::eDepth24);
    GfxPass::Attachment depth;
    depth.clear    = true;
    depth.depthVal = camera.isReverseZ() ? 0.f : 1.f;
    depth.image    = shadowMapImage;
    shadowGen      = get().getDevice().createPass({}, depth);
    buildTerrainDrawProgram();
  }

  camera.updateSunMatrix(sunRotation.get().toDir(), domeRadius);

  if (!shadowMat)
    shadowMat.emplace(*shadowProg);

  shadowMat->reset();
  uint32_t index = 0;
  shadowMat->pushScalar(index++, camera.getLightViewProj());
  shadowMat->pushTexture(index++, heights, {});
  shadowMat->pushScalar(index++, heightScale.get());
  shadowMat->pushScalar(index++, tileSize.x);
  shadowMat->pushScalar(index++, tileSize.y);
  shadowMat->pushScalar(index++, 1.f / (float)tileSize.x);
  shadowMat->pushScalar(index++, 1.f / (float)tileSize.y);

  GfxState state;
  state.blend[0].mode   = BlendMode::eDisabled;
  state.depthTest       = camera.isReverseZ() ? DepthTestMode::eGreaterEq : DepthTestMode::eLessEq;
  state.scissorsEnabled = false;
  state.viewport.size   = shadowMapRez;
  state.polygonOffset   = true;
  state.polyOffSlope    = camera.isReverseZ() ? -2.1f : 2.0f;
  state.polyOffBias     = camera.isReverseZ() ? -0.1f : 0.1f;
  app.getDevice()->setState(state);

  app.getDevice()->beginPass(shadowGen);
  app.getDevice()->draw(drawCall, shadowMat->program.material, shadowMat->data);
  app.getDevice()->endPass();

  shadowMapDirty = false;
}

void MeshPreview::buildShadowMapProgram()
{
  auto builder = app().getDevice()->createSourceBuilder(ShaderLang::eGLSL, SourceType::eShaderProgram);
  builder->param("shadow_view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->param("heights",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
  builder->param("hscale", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->param("width", DataFormat(DataType::eUint, DataType::eUint));
  builder->param("height", DataFormat(DataType::eUint, DataType::eUint));
  builder->param("rwidth", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->param("rheight", DataFormat(DataType::eFloat, DataType::eFloat));

  auto code = fileContentToString("shaders/shadow.glsl");
  builder->append(code);
  shadowProg = builder->finalize();
  assert(shadowProg->material.program);
}

void MeshPreview::drawTerrain(TerraMainApp const& app)
{
  if (!terrainMat)
    terrainMat.emplace(*materialProg);

  terrainMat->reset();
  uint32_t index   = 0;
  auto     sunDir  = sunRotation.get().toDir();
  auto     hrange  = pipeline->minMax();
  auto     hfactor = (hrange.y - hrange.x);
  hfactor          = hfactor > 0.f ? 1.f / hfactor : 0.f;
  terrainMat->pushScalar(index++, camera.getLightViewProjBias());
  terrainMat->pushScalar(index++, camera.getViewProj());
  terrainMat->pushScalar(index++, layerWeights.get());
  terrainMat->pushScalar(index++, vec4(sunDir, sunIntensity.get()));
  terrainMat->pushScalar(index++, vec2(hrange.x, hfactor));
  terrainMat->pushTexture(index++, heights, {});
  terrainMat->pushTexture(index++, terrainColors, layerSampler);
  terrainMat->pushTexture(index++, shadowMapImage, shadowSampler);
  terrainMat->pushTexture(index++, waterContrib, layerSampler);
  terrainMat->pushTexture(index++, vegetationContrib, layerSampler);
  terrainMat->pushTexture(index++, rocksContrib, layerSampler);
  terrainMat->pushScalar(index++, heightScale.get());
  terrainMat->pushScalar(index++, tileSize.x);
  terrainMat->pushScalar(index++, tileSize.y);
  terrainMat->pushScalar(index++, 1.f / (float)tileSize.x);
  terrainMat->pushScalar(index++, 1.f / (float)tileSize.y);

  GfxState state;
  state.blend[0].mode   = BlendMode::eDisabled;
  state.depthTest       = camera.isReverseZ() ? DepthTestMode::eGreaterEq : DepthTestMode::eLessEq;
  state.scissorsEnabled = false;
  state.viewport.size   = canvas.getSize();
  state.cullMode        = CullMode::eCullNone;
  app.getDevice()->setState(state);
  app.getDevice()->draw(drawCall, terrainMat->program.material, terrainMat->data);
}

void MeshPreview::buildTerrainDrawProgram()
{
  auto builder = app().getDevice()->createSourceBuilder(ShaderLang::eGLSL, SourceType::eShaderProgram);
  shadowProgOptions.setOption((uint32_t)shadowMapResolution.get());

  builder->options(shadowProgOptions);
  builder->param("shadow_view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->param("view_projection", DataFormat(DataType::eMat4, DataType::eMat4));
  builder->param("layer_weights", DataFormat(DataType::eFloat4, DataType::eFloat4));
  builder->param("sun_data", DataFormat(DataType::eFloat4, DataType::eFloat4));
  builder->param("hrange", DataFormat(DataType::eFloat2, DataType::eFloat2));
  builder->param("heights",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
  builder->param("layer_colors",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eRgba32f, ParamDeclType::eSampler1DArray));
  builder->param("shadow_map",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2DShadow));
  builder->param("water",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
  builder->param("vegetation",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
  builder->param("rocks",
                 DataFormat(DataType::eImage, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D));
  builder->param("hscale", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->param("width", DataFormat(DataType::eUint, DataType::eUint));
  builder->param("height", DataFormat(DataType::eUint, DataType::eUint));
  builder->param("rwidth", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->param("rheight", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->output("color_buffer",
                  DataFormat(DataType::eImage, DataType::eFloat4, ImageFormat::eRgba8, ParamDeclType::eSampler2D));

  auto code = fileContentToString("shaders/terrain.glsl");
  builder->append(code);
  materialProg = builder->finalize();
  assert(materialProg->material.program);
}

void MeshPreview::drawAtmosphere(TerraMainApp const& app)
{
  if (!atmosphereMat)
    atmosphereMat.emplace(*atmosphereProg);

  atmosphereMat->reset();
  uint32_t index  = 0;
  auto     sunDir = sunRotation.get().toDir();
  atmosphereMat->pushScalar(index++, planetScale.get());
  atmosphereMat->pushScalar(index++, vec4(sunDir * domeRadius * 10.f, sunIntensity.get()));

  GfxState state;
  state.blend[0].mode   = BlendMode::eDisabled;
  state.depthTest       = DepthTestMode::eDisabled;
  state.scissorsEnabled = false;
  state.viewport.size   = canvas.getSize();
  state.cullMode        = CullMode::eCullNone;
  app.getDevice()->setState(state);
  app.getDevice()->postProcessDraw(atmosphereMat->program.material.program, atmosphereMat->program.material.layout,
                                   atmosphereMat->data);
}

void MeshPreview::buildScatterProgram()
{
  auto builder = app().getDevice()->createSourceBuilder(ShaderLang::eGLSL, SourceType::ePostProcess);
  shadowProgOptions.setOption((uint32_t)shadowMapResolution.get());

  builder->options(shadowProgOptions);
  builder->param("scale", DataFormat(DataType::eFloat, DataType::eFloat));
  builder->param("sun_data", DataFormat(DataType::eFloat4, DataType::eFloat4));
  builder->output("color_buffer",
                  DataFormat(DataType::eImage, DataType::eFloat4, ImageFormat::eRgba8, ParamDeclType::eSampler2D));

  auto code = fileContentToString("shaders/scatter.glsl");
  builder->append(code);
  atmosphereProg = builder->finalize();
  assert(atmosphereProg->material.program);
}

void MeshPreview::tick()
{
  if (pipeline)
    shadowMapDirty |= pipeline->tick();
}

void MeshPreview::update(glm::ivec2 viewportSize, MouseState& ms)
{
  updateSunDir(viewportSize, ms);
  camera.update(viewportSize, box, sunRotation.get(), ms);
}
} // namespace terra
