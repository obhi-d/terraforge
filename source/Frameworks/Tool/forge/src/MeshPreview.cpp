
#include "MeshPreview.h"
#include "ImageSerializer.h"
#include "ResourceUtils.h"
#include "TerraMainApp.h"

namespace tmpl
{
#include "glsl/mesh.glsl"
}

namespace terra
{

static constexpr size_t UboSize = 2 * sizeof(glm::mat4) + sizeof(glm::vec4);

MeshPreview::MeshPreview()
{
  heightTexPath->path = (getMediaPath() / heightTexPath->path).string();
}

void MeshPreview::init(TerraMainApp& app)
{
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
  app.getDevice()->destroy(vertex);
  app.getDevice()->destroy(index);
  app.getDevice()->destroy(material.program);
  app.getDevice()->destroy(material.descriptorSet);
  app.getDevice()->destroy(GfxImage2D::handle(heightTexPath->image));
}

void MeshPreview::regenerate(TerraMainApp const& app, dshandle iactor)
{
  AppSettings const& settings = app.getSettings();
  if (!generated)
  {
    createDeviceObjects(app, *app.getDevice());
    generated = true;
  }

  if (settings.tileSize != tileSize || settings.nbPreviewTiles != nbPreviewTiles || !vertex || !index)
  {
    tileSize       = settings.tileSize;
    nbPreviewTiles = settings.nbPreviewTiles;
    vertexCount    = (tileSize.x * nbPreviewTiles.x) * (tileSize.y * nbPreviewTiles.y);
    box.x          = (float)(tileSize.x * nbPreviewTiles.x) + 1.f;
    box.y          = max - min + 1.f;
    box.z          = (float)(tileSize.y * nbPreviewTiles.y) + 1.f;
    uint32_t size  = vertexCount * 4;
    if (vertex)
      app.getDevice()->destroy(vertex);
    if (!settings.hasRobustAccess)
      size *= 2;
    vertex = app.getDevice()->createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::fVertex, size);
    if (index)
      app.getDevice()->destroy(index);
    auto patchX    = (uint32_t)((tileSize.x * nbPreviewTiles.x) - 1);
    auto patchY    = (uint32_t)((tileSize.y * nbPreviewTiles.y) - 1);
    auto indexSize = (uint32_t)((patchX * patchY + patchX + patchY) * 12 * sizeof(uint32_t));
    index = app.getDevice()->createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::fIndex, indexSize);

    auto indices = (uint32_t*)app.getDevice()->mapBuffer(index, 0, indexSize);
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

    uint32_t offset = patchX * patchY * 6;
    for (uint32_t y = 0; y < patchY; ++y)
    {
      auto patchId = uint32_t(y * 12 + offset);
      auto vertId  = uint32_t(y * (patchX + 1));
      // 004, 404

      indices[patchId + 0] = static_cast<uint32_t>(vertId);
      indices[patchId + 1] = static_cast<uint32_t>(vertId + vertexCount);
      indices[patchId + 2] = static_cast<uint32_t>(vertId + patchX + 1);

      indices[patchId + 3] = static_cast<uint32_t>(vertId + patchX + 1);
      indices[patchId + 4] = static_cast<uint32_t>(vertId + vertexCount);
      indices[patchId + 5] = static_cast<uint32_t>(vertId + patchX + 1 + vertexCount);

      indices[patchId + 6] = static_cast<uint32_t>(vertId + patchX);
      indices[patchId + 7] = static_cast<uint32_t>(vertId + patchX + patchX + 1);
      indices[patchId + 8] = static_cast<uint32_t>(vertId + patchX + vertexCount);

      indices[patchId + 9]  = static_cast<uint32_t>(vertId + patchX + vertexCount);
      indices[patchId + 10] = static_cast<uint32_t>(vertId + patchX + patchX + 1);
      indices[patchId + 11] = static_cast<uint32_t>(vertId + patchX + patchX + 1 + vertexCount);
    }

    offset += patchY * 12;
    auto lastLine = vertexCount - (patchX + 1);
    for (uint32_t x = 0; x < patchX; ++x)
    {
      auto patchId = uint32_t(x * 12 + offset);
      auto vertId  = uint32_t(x);
      // 004, 404

      indices[patchId + 0] = static_cast<uint32_t>(vertId);
      indices[patchId + 1] = static_cast<uint32_t>(vertId + 1);
      indices[patchId + 2] = static_cast<uint32_t>(vertId + vertexCount);

      indices[patchId + 3] = static_cast<uint32_t>(vertId + vertexCount);
      indices[patchId + 4] = static_cast<uint32_t>(vertId + 1);
      indices[patchId + 5] = static_cast<uint32_t>(vertId + 1 + vertexCount);

      indices[patchId + 6] = static_cast<uint32_t>(vertId + lastLine);
      indices[patchId + 8] = static_cast<uint32_t>(vertId + lastLine + 1);
      indices[patchId + 7] = static_cast<uint32_t>(vertId + lastLine + vertexCount);

      indices[patchId + 9]  = static_cast<uint32_t>(vertId + lastLine + vertexCount);
      indices[patchId + 11] = static_cast<uint32_t>(vertId + lastLine + 1);
      indices[patchId + 10] = static_cast<uint32_t>(vertId + lastLine + 1 + vertexCount);
    }

    offset += patchX * 12;
    for (uint32_t y = 0; y < patchY; ++y)
    {
      for (uint32_t x = 0; x < patchX; ++x)
      {
        auto patchId = offset + uint32_t(y * patchX + x) * 6;
        auto vertId  = uint32_t(y * (patchX + 1) + x) + vertexCount;

        indices[patchId + 0] = static_cast<uint32_t>(vertId);
        indices[patchId + 1] = static_cast<uint32_t>(vertId + 1);
        indices[patchId + 2] = static_cast<uint32_t>(vertId + patchX + 1);

        indices[patchId + 3] = static_cast<uint32_t>(vertId + patchX + 1);
        indices[patchId + 4] = static_cast<uint32_t>(vertId + 1);
        indices[patchId + 5] = static_cast<uint32_t>(vertId + patchX + 2);
      }
    }

    app.getDevice()->unmapBuffer(index);

    drawCall.indexBuffer.handle      = index;
    drawCall.indexBufferStride       = 4;
    drawCall.indexCount              = (patchX * patchY + patchX + patchY) * 12;
    drawCall.layout                  = layout;
    drawCall.type                    = GfxMesh::eTriangles;
    drawCall.vertexBuffers[0].handle = vertex;
    drawCall.vertexCount             = size / 4;
  }
  actor              = iactor;
  params.frequency   = settings.frequency;
  params.seed        = settings.seed;
  params.tileSize[0] = settings.tileSize->x;
  params.tileSize[1] = settings.tileSize->y;
  params.wavelength  = 1 / params.frequency;
  if (!pipeline)
  {
    pipeline = get().createPipeline();
  }

  if (actor)
  {

    pipeline->compute(actor, params, ivec2{settings.tileOffset->x, settings.tileOffset->y},
                      ivec2{tileSize.x * nbPreviewTiles.x, tileSize.y * nbPreviewTiles.y});
  }
  else
  {
    auto size     = (tileSize.x * nbPreviewTiles.x) * (tileSize.y * nbPreviewTiles.y) * 4;
    auto vertices = (float*)app.getDevice()->mapBuffer(vertex, 0, size);
    std::memset(vertices, 0, size);
    app.getDevice()->unmapBuffer(vertex);
  }
}

void MeshPreview::createDeviceObjects(TerraMainApp const& app, GfxDevice43& dev)
{
  if (!ubo)
  {
    ubo = dev.createBuffer(GfxStorageClass::eDynamicDeviceReadonly, GfxBuffer::fUniform, UboSize);
  }

  auto builder = dev.createShaderBuilder(terra::ShaderLang::eGLSL);
  builder->beginSection(ShaderBuilder::eDecl);
  std::array<GfxDescriptorSetLayout::Descriptor, 2> descriptors;
  auto                                              bindingInfo = builder->declConstants("U", "Params");
  descriptors[0]                                                = bindingInfo.descriptor;
  builder->append(bindingInfo.content);
  builder->append("{ ");
  builder->append(tmpl::gs_MeshUBO);
  builder->append("}constants;");
  builder->endSection();

  builder->begin(ShaderType::eVertex);
  builder->append(tmpl::gs_MeshVS);
  builder->end();

  builder->begin(ShaderType::eFragment);
  bindingInfo    = builder->declTexture("height_colors");
  descriptors[1] = bindingInfo.descriptor;
  builder->append(bindingInfo.content);
  builder->append(";\n");
  builder->append(tmpl::gs_MeshFS);
  builder->end();
  material.program         = dev.createProgram(ShaderOptions{}, *builder);
  sampler                  = dev.createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
  auto descriptorSetLayout = dev.createDescriptorSetLayout(descriptors);
  dev.applyLayoutToProgram(material.program, descriptorSetLayout);
  material.descriptorSet = dev.createDescriptorSet(descriptorSetLayout);
  GfxMesh::Layout mesh;
  mesh.vertexBufferCount             = 1;
  mesh.vertexBuffers[0].elementCount = 1;
  mesh.vertexBuffers[0].elements[0] =
    GfxMesh::VertexElement{.format = GfxVertexFormat::eFloat, .relOffset = 0, .shaderBinding = 0};
  mesh.vertexBuffers[0].stride = sizeof(float);
  layout                       = dev.createMeshLayout(mesh);
  reloadTexture(app);
}

void MeshPreview::reloadTexture(TerraMainApp const& app)
{
  heightTexPath->reload(app);
}

void MeshPreview::draw(Rect const& viewport, Rect const& scissor, TerraMainApp& app)
{
  GlGfxState state;

  if (pipeline->hasResults())
  {
    auto size     = (tileSize.x * nbPreviewTiles.x) * (tileSize.y * nbPreviewTiles.y) * 4;
    auto vertices = (float*)app.getDevice()->mapBuffer(vertex, 0, size);
    pipeline->getResults(vertices, size, min, max);
    app.getDevice()->unmapBuffer(vertex);
  }

  state.blend           = BlendMode::eDisabled;
  state.depthTest       = DepthTestMode::eLessEq;
  state.scissorsEnabled = true;
  state.viewport        = scissor;
  state.scissor         = scissor;

  app.getDevice()->setState(state);
  app.getDevice()->clearBackbuffer(glm::vec4(app.getTheme().themeColors.clear), true);
  AppSettings const& settings = app.getSettings();
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
  }
}

} // namespace terra
