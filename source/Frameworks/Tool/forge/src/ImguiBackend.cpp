
#include "Logger.h"
#include "ImguiBackend.h"
#include "IconsFontAwesome6.h"
#include "ResourceUtils.h"
#include "ImageSerializer.h"
#include <SDL.h>

namespace tmpl
{
  #include "glsl/draw2d.glsl"
}

namespace terra
{

void ImguiBackend::init(std::shared_ptr<GfxDevice43> renderer)
{
  this->renderer = renderer;
  auto&            io               = ImGui::GetIO();
  ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
  platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void* backend)
  {
    auto self = (ImguiBackend*)backend;    
    self->renderer->flushStates();
    self->draw(glm::vec2(viewport->Size.x,viewport->Size.y) , viewport->DrawData);
  };
  createDeviceObjects();
  state.blend = BlendMode::eAdditive;
  state.depthTest = DepthTestMode::eDisabled;
  state.scissorsEnabled = true;
}
void ImguiBackend::destroy() 
{
  renderer->destroy(params);
  renderer->destroy(font);
  renderer->destroy(image);
  renderer->destroy(sampler);
  renderer->destroy(descriptorSet);
  renderer->destroy(descriptorSetLayout);
  renderer->destroy(vertexData);
  renderer->destroy(indexData);
  renderer->destroy(effect);
  renderer->destroy(layout);

  ImGui::DestroyPlatformWindows();
}
void ImguiBackend::applyTheme(ImguiTheme const& theme)
{
  renderer->destroy(font);
  renderer->destroy(image);
  colors         = theme.themeColors;
  paramData.tint = theme.themeColors.tint;
  clearColor     = theme.themeColors.clear;
  auto& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text] = theme.themeColors.text;
  uploadFonts(theme);
}
void ImguiBackend::draw() 
{
  auto& io                = ImGui::GetIO();
  
  draw(glm::vec2(io.DisplaySize.x, io.DisplaySize.y), ImGui::GetDrawData());
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    SDL_Window*   backup_current_window  = SDL_GL_GetCurrentWindow();
    SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, this);
    SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
  }

}
void ImguiBackend::uploadFonts(ImguiTheme const& theme) 
{
  ImGuiIO& io = ImGui::GetIO();
  
  io.Fonts->Clear();
  {
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    auto font                       = fileContentToBytes(theme.images[ImageName::eFont].path);
    io.Fonts->AddFontFromMemoryTTF(reinterpret_cast<char*>(font.data()), (int)font.size(), 
      (float)theme.images[ImageName::eFont].size.y, &fontConfig);
  }
  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode            = true;
    config.GlyphMinAdvanceX     = 13.0f;

    static const ImWchar ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    auto font = fileContentToBytes(theme.images[ImageName::eIconFont].path);
    io.Fonts->AddFontFromMemoryTTF(reinterpret_cast<char*>(font.data()), (int)font.size(), 
      (float)theme.images[ImageName::eIconFont].size.y, &config,
                                   ranges);
  }
  if (!io.Fonts->Build())
  {
    throw std::runtime_error("Failed to build fonts.");
  }
  unsigned char* pixels = nullptr;
  int            width  = 0;
  int height = 0;

  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
  font = renderer->createImage(GfxStorageClass::eStaticDeviceReadonly, (uint32)width, (uint32)height,
                                    ImageFormat::eUnorm8,
                        (std::byte const*)pixels,
                        GfxImage2D::Swizzle{.r = GfxImage2D::ComponentValue::eOne,
                                            .g = GfxImage2D::ComponentValue::eOne,
                                            .b = GfxImage2D::ComponentValue::eOne,
                                            .a = GfxImage2D::ComponentValue::eRed});
  io.Fonts->SetTexID((ImTextureID)0);
  io.Fonts->ClearTexData();
  // build icon atlas
  uint32_t area = 0;
  std::array<PackInfo, ImagePackCount> packs;
  for (uint32 i = 2; i < theme.images.size(); ++i)
  {
    auto const& pack = theme.images[i];
    if (pack.size.x < 1 || pack.size.y < 1)
      continue;
    area += pack.size.x * pack.size.y;
  }
  auto dim = std::sqrt(area);
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t maxWidth  = 0;
  uint32_t maxHeight = 0;
  for (uint32 i = 2; i < theme.images.size(); ++i)
  {
    auto const& src  = theme.images[i];
    auto&       pack = packs[i];
    if (pack.size.x < 1 || pack.size.y < 1)
      continue;
    pack.offset.x = x + 1;
    pack.offset.y = y + 1;
    pack.size     = src.size;
    x += (src.size.x + 2);
    maxHeight = std::max<uint32_t>(y + src.size.y + 2, maxHeight);
    maxWidth  = std::max<uint32_t>(x, maxWidth);
    if (x > dim)
    {
      y = maxHeight;
      x = 0;
    }
  }
  if (maxWidth > 1 && maxHeight > 1)
  {
    ImageSerializer              serializer;
    std::unique_ptr<std::byte[]> packPixels = std::make_unique<std::byte[]>(maxWidth * maxHeight * 4);
    std::memset(packPixels.get(), 0, maxWidth * maxHeight * 4);
    for (uint32 i = 2; i < theme.images.size(); ++i)
    {
      auto& pack       = packs[i];
      packUVs[i].uv0.x = (float)(pack.offset.x) / (float)maxWidth;
      packUVs[i].uv0.y = (float)(pack.offset.y) / (float)maxHeight;
      packUVs[i].uv1.x = packUVs[i].uv0.x + (float)(pack.size.x) / (float)maxWidth;
      packUVs[i].uv1.y = packUVs[i].uv0.y + (float)(pack.size.y) / (float)maxHeight;
      if (pack.size.x < 1 || pack.size.y < 1)
        continue;

      std::vector<std::byte*> rows(pack.size.y);
      for (uint32 r = 0; r < pack.size.y; ++r)
        rows[r] = packPixels.get() + ((pack.offset.y + r) * maxWidth + pack.offset.x) * 4;
      serializer.loadImageRgba(rows, pack.size.y, pack.size.x, getMediaPath() / theme.images[i].path);
    }
    image = renderer->createImage(GfxStorageClass::eStaticDeviceReadonly, (uint32)maxWidth, (uint32)maxHeight,
                                  ImageFormat::eRgba8, packPixels.get());
  }
}

void ImguiBackend::createDeviceObjects() 
{
  auto builder = renderer->createShaderBuilder(terra::ShaderLang::eGLSL);
  builder->beginSection(ShaderBuilder::eDecl);
  std::array<GfxDescriptorSetLayout::Descriptor, 2> descriptors;
  auto                                              bindingInfo = builder->declConstants("U", "Params");
  descriptors[0]                                                = bindingInfo.descriptor;
  builder->append(bindingInfo.content);
  builder->append("{ ");
  builder->append(tmpl::gs_2dDecl);
  builder->append("}params;");
  builder->endSection();
  builder->begin(ShaderType::eVertex);
  builder->append(tmpl::gs_2dVS);
  builder->end();
  builder->begin(ShaderType::eFragment);
  bindingInfo = builder->declTexture("diffuse");
  descriptors[1] = bindingInfo.descriptor;
  builder->append(bindingInfo.content);
  builder->append(";\n");
  builder->append(tmpl::gs_2dFS); 
  builder->end();
  effect = renderer->createProgram(ShaderOptions{}, *builder);
  sampler = renderer->createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
  descriptorSetLayout = renderer->createDescriptorSetLayout(descriptors);
  renderer->applyLayoutToProgram(effect, descriptorSetLayout);
  descriptorSet = renderer->createDescriptorSet(descriptorSetLayout);
  GfxMesh::Layout mesh;
  mesh.vertexBufferCount             = 1;
  mesh.vertexBuffers[0].elementCount = 3;
  mesh.vertexBuffers[0].elements[0] =
    GfxMesh::VertexElement{.format = GfxVertexFormat::eFloat2, .relOffset = 0, .shaderBinding = 0};
  mesh.vertexBuffers[0].elements[1] =
    GfxMesh::VertexElement{.format = GfxVertexFormat::eFloat2, .relOffset = 8, .shaderBinding = 1};
  mesh.vertexBuffers[0].elements[2] =
    GfxMesh::VertexElement{.format = GfxVertexFormat::eUnormByte4, .relOffset = 16, .shaderBinding = 2};
  mesh.vertexBuffers[0].stride = sizeof(ImDrawVert);
  layout                       = renderer->createMeshLayout(mesh);
  params = renderer->createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::fUniform, sizeof(Params));
  this->descriptors[0].first = params;
}

void ImguiBackend::draw(glm::vec2 frameSize, ImDrawData* data)
{
  if (!pendingDeletion.empty())
  {
    for (auto b : pendingDeletion)
      renderer->destroy(b);
    pendingDeletion.clear();
  }
  state.viewport.offset.x = 0;
  state.viewport.offset.y = 0;
  state.viewport.size.x   = (gl::GLsizei)frameSize.x;
  state.viewport.size.y   = (gl::GLsizei)frameSize.y;
  state.scissor           = state.viewport;

  renderer->setState(state);
  renderer->clearBackbuffer(clearColor);
  

  uint32_t vertexDataOffset = 0;
  uint32_t indexDataOffset  = 0;
  float    L                = data->DisplayPos.x;
  float    R                = data->DisplayPos.x + data->DisplaySize.x;
  float    T                = data->DisplayPos.y;
  float    B                = data->DisplayPos.y + data->DisplaySize.y;
  paramData.projection = glm::mat4({2.0f / (R - L), 0.0f, 0.0f, 0.0f}, {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
                                   {0.0f, 0.0f, -1.0f, 0.0f}, {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f});
  auto paramDataPtr = renderer->mapBuffer(params, 0, sizeof(Params));
  std::memcpy(paramDataPtr, &paramData, sizeof(Params));
  renderer->unmapBuffer(params);
  int fbHeight = (int)(data->DisplaySize.y * data->FramebufferScale.y);
  createBuffers(data);
  ImVec2 clipOff   = data->DisplayPos;       // (0,0) unless using multi-viewports
  ImVec2 clipScale = data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
  GfxMesh::Draw draw;
  draw.layout                  = layout;
  draw.type                    = GfxMesh::eTriangles;
  draw.indexBuffer.handle      = indexData;
  draw.vertexBuffers[0].handle = vertexData;
  draw.indexBufferStride       = sizeof(ImDrawIdx);
  GfxMaterial material{.program = effect, .descriptorSet = descriptorSet};
  for (int n = 0; n < data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];
      ImVec2 clipMin((pcmd.ClipRect.x - clipOff.x) * clipScale.x, (pcmd.ClipRect.y - clipOff.y) * clipScale.y);
      ImVec2 clipMax((pcmd.ClipRect.z - clipOff.x) * clipScale.x, (pcmd.ClipRect.w - clipOff.y) * clipScale.y);
      if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
        continue;
      state.scissor.offset.x = (int)clipMin.x;
      state.scissor.offset.y = (fbHeight - (int)clipMax.y);
      state.scissor.size.x   = (int)(clipMax.x - clipMin.x);
      state.scissor.size.y   = (int)(clipMax.y - clipMin.y);
      renderer->setState(state);
      auto id = (uint32_t)(uintptr_t)pcmd.GetTexID();
      if (id == 0 || id == 1)
        descriptors[1].first = font;
      else
        descriptors[1].first = image;      
      descriptors[1].second = sampler;
      renderer->updateDescriptorSet(descriptorSet, descriptors);
      draw.baseVertex         = pcmd.VtxOffset;
      draw.vertexBuffers[0].offset = vertexDataOffset;
      draw.indexBuffer.offset      = indexDataOffset + (pcmd.IdxOffset * sizeof(ImDrawIdx));
      draw.indexCount         = pcmd.ElemCount;
      renderer->draw(draw, material);
    }

    vertexDataOffset += cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
    indexDataOffset += cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);  

  }
}

void ImguiBackend::createBuffers(ImDrawData* data) 
{
  uint32_t vertexBufferSize = 0;
  uint32_t indexBufferSize  = 0;
  for (int n = 0; n < data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = data->CmdLists[n];
  
    vertexBufferSize += cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
    indexBufferSize += cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);  
  }

  if (vertexBufferSize  > vertexDataSize)
  {
    pendingDeletion.push_back(vertexData);
    vertexData = renderer->createBuffer(GfxStorageClass::eDynamicDeviceReadonly, GfxBuffer::fVertex,
                           vertexBufferSize );
    vertexDataSize = vertexBufferSize ;
  }

  if (indexBufferSize > indexDataSize)
  {
    pendingDeletion.push_back(indexData);
    indexData = renderer->createBuffer(GfxStorageClass::eDynamicDeviceReadonly, GfxBuffer::fIndex,
                           indexBufferSize );
    indexDataSize = indexBufferSize;
  }

  if (!vertexBufferSize || !indexBufferSize)
    return;
  auto vertexDataPtr = renderer->mapBuffer(vertexData, 0, vertexBufferSize);
  auto indexDataPtr  = renderer->mapBuffer(indexData, 0, indexBufferSize);
  for (int n = 0; n < data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = data->CmdLists[n];

    vertexBufferSize = cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
    indexBufferSize = cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);  

    std::memcpy(vertexDataPtr, cmd_list->VtxBuffer.Data, vertexBufferSize);
    std::memcpy(indexDataPtr, cmd_list->IdxBuffer.Data, indexBufferSize);

    vertexDataPtr += vertexBufferSize;
    indexDataPtr += indexBufferSize;
  }
  renderer->unmapBuffer(vertexData);
  renderer->unmapBuffer(indexData);
}
///----------------------------------------------------------------------------
/// Draw Helpers
///----------------------------------------------------------------------------
ImAlign ImguiBackend::align(ImAlign align, float padding)
{
  auto l    = alignment;
  alignment = align;
  currentRegExtends.padding = padding;
  return l;
}
void ImguiBackend::imageIcon(ImageName name, ImVec2 size, Color tint)
{
  auto const& uv = packUVs[name];
  if (alignment == ImAlign::eRight)
  {
    auto x = currentRegExtends.right - (size.x + currentRegExtends.padding);
    ImGui::SetCursorPosX(x);
    currentRegExtends.right = x;
  }
  else
    currentRegExtends.left += (size.x + currentRegExtends.padding);
  ImGui::Image(toTexture(name), size, uv.uv0, uv.uv1, tint);
}
bool ImguiBackend::iconButton(std::string_view name, ImVec2 size, Color color, Color hover)
{
  bool clicked = false;
  auto pos = ImGui::GetCursorPos();
  if (alignment == ImAlign::eRight)
  {
    pos.x = currentRegExtends.right - (size.x + currentRegExtends.padding);
    ImGui::SetCursorPosX(pos.x);
  }
  
  std::string nameId = "##";
  nameId += name;
  if (ImGui::InvisibleButton(nameId.c_str(), size))
    clicked = true;

  if (ImGui::IsItemHovered())
  {
    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), hover);
  }

  ImGui::SameLine();
  textCentered(name.data(), pos, size);
  if (alignment == ImAlign::eRight)
    currentRegExtends.right -= (size.x + currentRegExtends.padding);
  else
    currentRegExtends.left += (size.x + currentRegExtends.padding);
  return clicked;
}

void ImguiBackend::textCentered(std::string_view text, ImVec2 pos, ImVec2 windowWidth)
{
  auto textDim     = ImGui::CalcTextSize(text.data());
  ImGui::SetCursorPos(ImVec2(pos.x + (windowWidth.x - textDim.x) * 0.5f, pos.y + (windowWidth.y - textDim.y) * 0.5f));
  ImGui::Text(text.data());
}
TitlebarAction ImguiBackend::beginTitlebar(ImVec2 size, ImWith flags)
{
  TitlebarAction name  = TitlebarAction::eNone;
  currentRegExtends.left = 0;
  currentRegExtends.right = size.x;
  ImGui::SetNextWindowSize(size);
  ImGui::Begin("Main", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoDocking |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  if (flags & ImWith::fLogo)
  {
    ImGui::SetCursorPos(ImVec2(4, 4));
    imageIcon(ImageName::eLogo, ImVec2(32, 32), colors.logo);
  }
  auto last = align(ImAlign::eRight);
  if (flags & ImWith::fClose)
  {
    ImGui::SameLine();
    if (iconButton(ICON_FA_XMARK, ImVec2(40, 30), colors.text, colors.iconHover))
      name = TitlebarAction::eClose;
  }
  if (flags & ImWith::fMaximize)
  {
    ImGui::SameLine();
    if (iconButton(ICON_FA_WINDOW_MAXIMIZE, ImVec2(40, 30), colors.text, colors.iconHover))
      name = TitlebarAction::eMaximize;
  }
  if (flags & ImWith::fRestore)
  {
    ImGui::SameLine();
    if (iconButton(ICON_FA_WINDOW_RESTORE, ImVec2(40, 30), colors.text, colors.iconHover))
      name = TitlebarAction::eRestore;
  }
  if (flags & ImWith::fMinimize)
  {
    ImGui::SameLine();
    if (iconButton(ICON_FA_WINDOW_MINIMIZE, ImVec2(40, 30), colors.text, colors.iconHover))
      name = TitlebarAction::eMinimize;
  }
  if (name == TitlebarAction::eNone)
  {
    ImGui::SameLine();
    ImGui::SetCursorPosX(currentRegExtends.left);
    ImGui::InvisibleButton("##MainTitle", ImVec2(currentRegExtends.right - currentRegExtends.left, size.y), 0);
    if (ImGui::IsItemHovered())
    {
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        name = TitlebarAction::eDrag;
      else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        name = TitlebarAction::eToggleSize;
    }
  }
  align(last);
  return name;
}
void ImguiBackend::endTitlebar() 
{
  ImGui::End();
}
bool ImguiBackend::drawResizeControl(glm::ivec2 windowSize) 
{
  ImGui::SetNextWindowPos(ImVec2(windowSize.x - 28.f, windowSize.y - 28.f));
  ImGui::SetNextWindowSize(ImVec2(20, 20));
  ImGui::Begin("Resize", nullptr,
               ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoDecoration);
  ImGui::InvisibleButton("##ResizeCtrl", ImVec2(20, 20), 0);
  bool isDragging = ImGui::IsItemClicked();
    
  ImGui::GetWindowDrawList()->AddTriangleFilled(ImVec2(windowSize.x - 4.f, windowSize.y - 20.f),
                                                ImVec2(windowSize.x - 4.f, windowSize.y - 4.f),
                                                ImVec2(windowSize.x - 20.f, windowSize.y - 4.f), colors.text);
  ImGui::End();
  return isDragging;
}
}