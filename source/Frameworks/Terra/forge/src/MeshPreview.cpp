
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
  app.dispatcher().remove(setActorEventListener);
  app.getDevice()->destroy(index);
  app.getDevice()->destroy(shadowMapImage);
  app.getDevice()->destroy(shadowMap);
  app.getDevice()->destroy(sampler);
  app.getDevice()->destroy(layout);
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
  sampler       = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
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
  app.getDevice()->destroy(terrainColors);
  terrainColors = app.getDevice()->create1DImageArray(GfxStorageClass::eStaticDeviceReadonly, Width, 4,
                                                      ImageFormatEnum::eRgba8, (ubyte_t const*)layerColors.data());
}

void MeshPreview::updateShadowMap(TerraMainApp const& app) 
{
  if (!shadowMapDirty)
    return;

  shadowMapDirty = false;
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

  pipeline->getResults(heights, layerContrib);
  if (!heights)
    heights = nullImage;
  if (!layerContrib)
    layerContrib = nullImage;

  state.blend[0].mode   = BlendMode::eDisabled;
  state.depthTest       = camera.isReverseZ() ? DepthTestMode::eGreaterEq : DepthTestMode::eLessEq;
  state.scissorsEnabled = true;
  state.viewport        = scissor;
  state.scissor         = scissor;

  app.getDevice()->setState(state);
  app.getDevice()->clearBackbuffer(glm::vec4(app.getTheme().themeColors.clear), 
    camera.isReverseZ() ? DepthClear::eClearZ_0 : DepthClear::eClearZ_1);
  AppSettings const& settings = app.getSettings();

  updateShadowMap(app);

  // update buffer
  int width  = tileSize.x * nbPreviewTiles.x;
  int height = tileSize.y * nbPreviewTiles.y;

  struct Data
  {
    glm::mat4 view_projection;
    int       width;
    int       height;
    float     style;
    float     frequency;
    glm::vec3 sun_dir;
    float     height_multiplier;
    glm::vec4 sun_color;
    glm::vec4 tint;
    int       vertexCount;
    float     max;
    float     min;
    float     crust;
  };

  static_assert(sizeof(Data) == UboSize);

  Data& data = *(Data*)app.getDevice()->mapBuffer(ubo, 0, UboSize);

  data.view_projection   = camera.getViewProj();
  data.width             = width;
  data.height            = height;
  data.style             = meshStyle;
  data.frequency         = settings.frequency;
  data.sun_dir           = sunRotation->toDir();
  data.height_multiplier = heightMultiplier;
  data.sun_color         = glm::vec4(sunColor->r(), sunColor->g(), sunColor->b(), sunIntensity.get());
  data.tint              = meshTint.get();
  data.vertexCount       = vertexCount;
  data.max               = max;
  data.min               = min;
  data.crust             = 1.f;
  app.getDevice()->unmapBuffer(ubo);

  if (descriptorsDirty)
  {
    std::array<GfxDescriptorSet::rhandle, 2> descriptors;
    descriptors[0].first  = ubo.um_index();
    descriptors[1].first  = heightTexPath->image;
    descriptors[1].second = sampler.um_index();
    app.getDevice()->updateDescriptorSet(material.descriptorSet, descriptors);
  }

  app.getDevice()->draw(drawCall, material);
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
  auto code = fileContentToString("shaders/shadow.glsl");
  builder->append(code);
  shadowProg = builder->finalize();
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
