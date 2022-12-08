
#include "ImguiBackend.h"
#include "CustomFont.cpp"
#include "IconsFontAwesome6.h"
#include "ImageSerializer.h"
#include "ImguiTerraWindow.h"
#include "Logger.h"
#include "ResourceUtils.h"
#include <SDL.h>

namespace tmpl
{
#include "glsl/draw2d.glsl"
}

namespace terra
{

void ImguiBackend::init(std::shared_ptr<GfxDevice43> renderer)
{
  this->renderer               = renderer;
  auto&            io          = ImGui::GetIO();
  ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
  platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void* backend)
  {
    auto self = (ImguiBackend*)backend;
    self->renderer->flushStates();
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
      self->renderer->clearBackbuffer(self->clearColor, true);
    self->draw(glm::vec2(viewport->Size.x, viewport->Size.y), viewport->DrawData);
  };
  createDeviceObjects();
  state.blend           = BlendMode::eAdditive;
  state.depthTest       = DepthTestMode::eDisabled;
  state.scissorsEnabled = true;
}
void ImguiBackend::destroy()
{
  renderer->destroy(params);
  renderer->destroy(font);
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
  this->theme                 = &theme;
  paramData.tint              = theme.themeColors.tint;
  clearColor                  = theme.themeColors.clear;
  auto& style                 = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text] = theme.themeColors.text;
  style.FramePadding.y *= 2;
  uploadFonts(theme);
}
void ImguiBackend::draw()
{
  auto& io = ImGui::GetIO();
  draw(glm::vec2(io.DisplaySize.x, io.DisplaySize.y), ImGui::GetDrawData());
}

void ImguiBackend::drawOtherWindows()
{
  auto& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    SDL_Window*   backup_current_window  = SDL_GL_GetCurrentWindow();
    SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, this);
  }
}

void ImguiBackend::uploadFonts(ImguiTheme const& theme)
{
  ImGuiIO& io = ImGui::GetIO();

  io.Fonts->Clear();

  /*if (std::filesystem::exists(getMediaPath() / theme.images[ImageName::eHeaderFont].path))
  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH          = 4;
    config.OversampleV          = 4;
    config.PixelSnapH           = false;
    auto font                   = fileContentToBytes(theme.images[ImageName::eHeaderFont].path);
    io.Fonts->AddFontFromMemoryTTF(reinterpret_cast<char*>(font.data()), (int)font.size(),
                                   (float)theme.images[ImageName::eHeaderFont].size.y, &config);
  }*/
  if (std::filesystem::exists(getMediaPath() / theme.images[ImageName::eFont].path))
  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH          = 4;
    config.OversampleV          = 4;
    config.PixelSnapH           = false;
    auto font                   = fileContentToBytes(theme.images[ImageName::eFont].path);
    io.Fonts->AddFontFromMemoryTTF(reinterpret_cast<char*>(font.data()), (int)font.size(),
                                   (float)theme.images[ImageName::eFont].size.y, &config);
  }
  if (std::filesystem::exists(getMediaPath() / theme.images[ImageName::eIconFont].path))
  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode            = true;
    config.GlyphMinAdvanceX     = 13.0f;
    config.OversampleH          = 4;
    config.OversampleV          = 4;
    config.PixelSnapH           = false;

    static const ImWchar ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    auto font = fileContentToBytes(theme.images[ImageName::eIconFont].path);
    io.Fonts->AddFontFromMemoryTTF(reinterpret_cast<char*>(font.data()), (int)font.size(),
                                   (float)theme.images[ImageName::eIconFont].size.y, &config, ranges);
  }

  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode            = true;
    config.GlyphMinAdvanceX     = 15.0f;
    config.OversampleH          = 4;
    config.OversampleV          = 4;
    config.PixelSnapH           = false;

    static const ImWchar ranges[] = {ICON_MIN_IGFD, ICON_MAX_IGFD, 0};

    io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_IGFD, 15.0f, &config, ranges);
  }

  ImageSerializer                 serializer;
  std::vector<ubyte_t>          imageData;
  std::array<int, ImagePackCount> packIDs;
  for (uint32 i = ImageName::eLogo; i < theme.images.size(); ++i)
  {
    auto const& img = theme.images[i];
    packIDs[i]      = io.Fonts->AddCustomRectRegular(img.size.x, img.size.y);
  }

  if (!io.Fonts->Build())
  {
    throw std::runtime_error("Failed to build fonts.");
  }
  unsigned char* pixels = nullptr;
  int            width  = 0;
  int            height = 0;

  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

  for (uint32 i = ImageName::eLogo; i < theme.images.size(); ++i)
  {
    auto const& img      = theme.images[i];
    auto        custRect = io.Fonts->GetCustomRectByIndex(packIDs[i]);
    if (!custRect)
      continue;
    packUVs[i].uv0.x = custRect->X / (float)width;
    packUVs[i].uv0.y = custRect->Y / (float)height;
    packUVs[i].uv1.x = (custRect->X + img.size.x) / (float)width;
    packUVs[i].uv1.y = (custRect->Y + img.size.y) / (float)height;

    std::vector<ubyte_t*> rows(img.size.y);
    for (uint32 r = 0; r < img.size.y; ++r)
      rows[r] = (ubyte_t*)(pixels + ((custRect->Y + r) * width + custRect->X));
    serializer.loadImageGray(rows, img.size.x, img.size.y, getMediaPath() / theme.images[i].path);
  }

  font      = renderer->create2DImage(GfxStorageClass::eStaticDeviceReadonly, (uint32)width, (uint32)height,
                                    ImageFormatEnum::eUnorm8, (ubyte_t const*)pixels,
                                    GfxImage::Swizzle{.r = GfxImage::ComponentValue::eOne,
                                                        .g = GfxImage::ComponentValue::eOne,
                                                        .b = GfxImage::ComponentValue::eOne,
                                                        .a = GfxImage::ComponentValue::eRed},
                                    1);
  whiteUV.x = io.Fonts->TexUvWhitePixel.x;
  whiteUV.y = io.Fonts->TexUvWhitePixel.y;
  io.Fonts->SetTexID((ImTextureID)0);
  io.Fonts->ClearTexData();
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
  bindingInfo    = builder->declTexture("diffuse");
  descriptors[1] = bindingInfo.descriptor;
  builder->append(bindingInfo.content);
  builder->append(";\n");
  builder->append(tmpl::gs_2dFS);
  builder->end();
  effect              = renderer->createProgram(ShaderOptions{}, *builder);
  sampler             = renderer->createSampler(ImageSampling(SamplingType::eLinear, Tiling::eClampToEdge));
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
  this->descriptors[0].first = params.um_index();
}

void ImguiBackend::draw(glm::vec2 frameSize, ImDrawData* data)
{
  if (!pendingDeletion.empty())
  {
    for (auto b : pendingDeletion)
      renderer->destroy(b);
    pendingDeletion.clear();
  }
  state.cullMode          = CullMode::eCullNone;
  state.viewport.offset.x = 0;
  state.viewport.offset.y = 0;
  state.viewport.size.x   = (gl::GLsizei)frameSize.x;
  state.viewport.size.y   = (gl::GLsizei)frameSize.y;
  state.scissor           = state.viewport;

  renderer->setState(state);

  uint32_t vertexDataOffset = 0;
  uint32_t indexDataOffset  = 0;
  float    L                = data->DisplayPos.x;
  float    R                = data->DisplayPos.x + data->DisplaySize.x;
  float    T                = data->DisplayPos.y;
  float    B                = data->DisplayPos.y + data->DisplaySize.y;
  paramData.projection      = glm::mat4({2.0f / (R - L), 0.0f, 0.0f, 0.0f}, {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
                                        {0.0f, 0.0f, -1.0f, 0.0f}, {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f});
  auto paramDataPtr         = renderer->mapBuffer(params, 0, sizeof(Params));
  std::memcpy(paramDataPtr, &paramData, sizeof(Params));
  renderer->unmapBuffer(params);
  int fbHeight = (int)(data->DisplaySize.y * data->FramebufferScale.y);
  createBuffers(data);
  ImVec2        clipOff   = data->DisplayPos;       // (0,0) unless using multi-viewports
  ImVec2        clipScale = data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
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

      if (pcmd.UserCallback)
      {
        auto data              = (ImguiBackend::CallbackData*)pcmd.UserCallbackData;
        data->viewport         = state.viewport;
        data->scissor.offset.x = (int)clipMin.x;
        data->scissor.offset.y = (fbHeight - (int)clipMax.y);
        data->scissor.size.x   = (int)(clipMax.x - clipMin.x);
        data->scissor.size.y   = (int)(clipMax.y - clipMin.y);
        pcmd.UserCallback(cmd_list, &pcmd);
        renderer->setState(state);
      }

      if (pcmd.ElemCount > 0)
      {
        state.scissorsEnabled  = true;
        state.scissor.offset.x = (int)clipMin.x;
        state.scissor.offset.y = (fbHeight - (int)clipMax.y);
        state.scissor.size.x   = (int)(clipMax.x - clipMin.x);
        state.scissor.size.y   = (int)(clipMax.y - clipMin.y);
        renderer->setState(state);
        auto id = (uint32_t)(uintptr_t)pcmd.GetTexID();
        if (id == 0)
          descriptors[1].first = font.um_index();
        else
          descriptors[1].first = id;
        descriptors[1].second = sampler.um_index();
        renderer->updateDescriptorSet(descriptorSet, descriptors);
        draw.baseVertex              = pcmd.VtxOffset;
        draw.vertexBuffers[0].offset = vertexDataOffset;
        draw.indexBuffer.offset      = indexDataOffset + (pcmd.IdxOffset * sizeof(ImDrawIdx));
        draw.indexCount              = pcmd.ElemCount;
        renderer->draw(draw, material);
      }
    }

    vertexDataOffset += cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
    indexDataOffset += cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
  }
  if (internalDrawIdx.empty())
    return;
  state.scissorsEnabled = false;
  renderer->setState(state);
  draw.baseVertex              = 0;
  draw.vertexBuffers[0].offset = vertexDataOffset;
  draw.indexBuffer.offset      = indexDataOffset;
  draw.indexCount              = (uint32_t)internalDrawIdx.size();
  renderer->draw(draw, material);
  internalDrawIdx.clear();
  internalDrawVtx.clear();
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

  vertexBufferSize += (uint32)internalDrawVtx.size() * sizeof(ImDrawVert);
  indexBufferSize += (uint32)internalDrawIdx.size() * sizeof(ImDrawIdx);

  if (vertexBufferSize > vertexDataSize)
  {
    pendingDeletion.push_back(vertexData);
    vertexData = renderer->createBuffer(GfxStorageClass::eDynamicDeviceReadonly, GfxBuffer::fVertex, vertexBufferSize);
    vertexDataSize = vertexBufferSize;
  }

  if (indexBufferSize > indexDataSize)
  {
    pendingDeletion.push_back(indexData);
    indexData     = renderer->createBuffer(GfxStorageClass::eDynamicDeviceReadonly, GfxBuffer::fIndex, indexBufferSize);
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
    indexBufferSize  = cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);

    std::memcpy(vertexDataPtr, cmd_list->VtxBuffer.Data, vertexBufferSize);
    std::memcpy(indexDataPtr, cmd_list->IdxBuffer.Data, indexBufferSize);

    vertexDataPtr += vertexBufferSize;
    indexDataPtr += indexBufferSize;
  }

  vertexBufferSize = (uint32)internalDrawVtx.size() * sizeof(ImDrawVert);
  indexBufferSize  = (uint32)internalDrawIdx.size() * sizeof(ImDrawIdx);
  std::memcpy(vertexDataPtr, internalDrawVtx.data(), vertexBufferSize);
  std::memcpy(indexDataPtr, internalDrawIdx.data(), indexBufferSize);

  renderer->unmapBuffer(vertexData);
  renderer->unmapBuffer(indexData);
}
///----------------------------------------------------------------------------
/// Draw Helpers
///----------------------------------------------------------------------------
void ImguiBackend::drawIcon(char16_t iconChar, glm::ivec2 location, glm::ivec2 size, Color color)
{
  auto& io   = ImGui::GetIO();
  auto  icon = io.Fonts->Fonts[0]->FindGlyph(iconChar);
  if (!icon)
    return;
  auto loc = currentRegExtends.min + location;
  size.y   = (int)(icon->Y1 - icon->Y0);
  loc.y -= size.y / 2;
  pushQuad(loc, size, glm::vec2(icon->U0, icon->V0), glm::vec2(icon->U1, icon->V1), color);
}
void ImguiBackend::drawIcon(ImageName iconChar, glm::ivec2 location, glm::ivec2 size, Color color)
{
  auto& io = ImGui::GetIO();
  pushQuad(currentRegExtends.min + location, size, packUVs[iconChar].uv0, packUVs[iconChar].uv1, color);
}
void ImguiBackend::pushQuad(glm::ivec2 loc, glm::ivec2 size, glm::vec2 uv0, glm::vec2 uv1, Color color)
{
  auto     max   = loc + size;
  uint16_t index = (uint16_t)internalDrawVtx.size();
  internalDrawVtx.push_back(ImDrawVert{.pos = ImVec2(loc.x, loc.y), .uv = ImVec2(uv0.x, uv0.y), .col = color});
  internalDrawVtx.push_back(ImDrawVert{.pos = ImVec2(loc.x, max.y), .uv = ImVec2(uv0.x, uv1.y), .col = color});
  internalDrawVtx.push_back(ImDrawVert{.pos = ImVec2(max.x, max.y), .uv = ImVec2(uv1.x, uv1.y), .col = color});
  internalDrawVtx.push_back(ImDrawVert{.pos = ImVec2(max.x, loc.y), .uv = ImVec2(uv1.x, uv0.y), .col = color});
  internalDrawIdx.emplace_back(index + 0);
  internalDrawIdx.emplace_back(index + 1);
  internalDrawIdx.emplace_back(index + 2);
  internalDrawIdx.emplace_back(index + 2);
  internalDrawIdx.emplace_back(index + 3);
  internalDrawIdx.emplace_back(index + 0);
}

void ImguiBackend::setRegion(glm::ivec2 start, glm::ivec2 size)
{
  currentRegExtends.min = start;
  currentRegExtends.max = start + size;
}
bool ImguiBackend::isIntersecting()
{
  auto& io = ImGui::GetIO();
  return (currentRegExtends.min.x <= (int)io.MousePos.x && (int)io.MousePos.x <= currentRegExtends.max.x) &&
         (currentRegExtends.min.y <= (int)io.MousePos.y && (int)io.MousePos.y <= currentRegExtends.max.y);
}

ImAlign ImguiBackend::setLayout(glm::ivec2 start, glm::ivec2 size, ImAlign align, float padding)
{
  auto l                    = alignment;
  alignment                 = align;
  currentRegExtends.min     = start;
  currentRegExtends.max     = start + size;
  currentRegExtends.padding = padding;
  return l;
}
ImAlign ImguiBackend::align(ImAlign al)
{
  auto l    = alignment;
  alignment = al;
  return l;
}

bool ImguiBackend::iconButton(std::string_view name, ImVec2 size, Color color, Color hover)
{
  bool clicked = false;
  auto pos     = ImGui::GetCursorPos();
  if (alignment == ImAlign::eRight)
  {
    pos.x = currentRegExtends.max.x - (size.x + currentRegExtends.padding);
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
    currentRegExtends.max.x -= int(size.x + currentRegExtends.padding);
  else
    currentRegExtends.min.x += int(size.x + currentRegExtends.padding);
  return clicked;
}

std::tuple<bool, glm::ivec2, Color> ImguiBackend::iconButtonSetup(glm::ivec2 size, int iconSize, bool inlay,
                                                                  Color normal, Color hover, Color pressed)
{
  auto const& io      = ImGui::GetIO();
  bool        clicked = false;
  glm::ivec2  pos;
  pos.y = currentRegExtends.min.y + ((currentRegExtends.max.y - currentRegExtends.min.y) - size.y) / 2;
  Color sel;
  if (alignment == ImAlign::eRight)
  {
    pos.x = currentRegExtends.max.x - int(size.x + currentRegExtends.padding);
  }
  else
    pos.x = currentRegExtends.min.x + int(currentRegExtends.padding);

  if (isIntersecting(glm::ivec2((int)io.MousePos.x, (int)io.MousePos.y), pos, size))
  {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
      if (inlay)
        pushQuad(pos, size, whiteUV, whiteUV, pressed);
      sel     = pressed;
      clicked = true;
    }
    else
    {
      if (inlay)
        pushQuad(pos, size, whiteUV, whiteUV, hover);
      sel = hover;
    }
  }
  else
  {
    if (inlay)
      pushQuad(pos, size, whiteUV, whiteUV, normal);
    sel = theme->themeColors.icon;
  }
  if (alignment == ImAlign::eRight)
    currentRegExtends.max.x = pos.x;
  else
    currentRegExtends.min.x = pos.x;
  return {clicked, pos, sel};
}

bool ImguiBackend::iconButton(char16_t cc, glm::ivec2 size, int iconSize, Color normal, Color hover, Color pressed,
                              bool inlay)
{
  auto [clicked, pos, color] = iconButtonSetup(size, iconSize, inlay, normal, hover, pressed);
  glm::ivec2 ics(iconSize, 0);

  drawIcon(cc, pos + (size - ics) / 2 - currentRegExtends.min, ics, inlay ? theme->themeColors.text : color);
  return clicked;
}

bool ImguiBackend::iconButton(ImageName cc, glm::ivec2 size, int iconSize, Color normal, Color hover, Color pressed,
                              bool inlay)
{
  auto [clicked, pos, color] = iconButtonSetup(size, iconSize, inlay, normal, hover, pressed);
  glm::ivec2 ics(iconSize, iconSize);
  drawIcon(cc, pos + (size - ics) / 2 - currentRegExtends.min, ics, inlay ? theme->themeColors.text : color);
  return clicked;
}
void ImguiBackend::textCentered(std::string_view text, ImVec2 pos, ImVec2 windowWidth)
{
  auto textDim = ImGui::CalcTextSize(text.data());
  ImGui::SetCursorPos(ImVec2(pos.x + (windowWidth.x - textDim.x) * 0.5f, pos.y + (windowWidth.y - textDim.y) * 0.5f));
  ImGui::Text(text.data());
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
               ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration);
  ImGui::InvisibleButton("##ResizeCtrl", ImVec2(20, 20), 0);
  bool isDragging = ImGui::IsItemClicked();

  ImGui::GetWindowDrawList()->AddTriangleFilled(
    ImVec2(windowSize.x - 4.f, windowSize.y - 20.f), ImVec2(windowSize.x - 4.f, windowSize.y - 4.f),
    ImVec2(windowSize.x - 20.f, windowSize.y - 4.f), theme->themeColors.text);
  ImGui::End();
  return isDragging;
}

bool ImguiBackend::toggleButton(std::string_view name, bool& toggled, ImVec2 size, std::u8string_view tip,
                                float padding)
{
  bool        clicked = false;
  std::string nameAlt = "##";
  nameAlt += name;
  auto x = ImGui::GetCursorPosX();
  auto y = ImGui::GetCursorPosY();
  if (ImGui::InvisibleButton(nameAlt.c_str(), size))
    clicked = true;

  if (ImGui::IsItemHovered() && !tip.empty())
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
    ImGui::BeginTooltip();
    ImGui::TextUnformatted((const char*)tip.data());
    ImGui::EndTooltip();
    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
  }
  else if (toggled)
  {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  }
  else
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));

  ImGui::SameLine();
  if (padding)
    ImGui::SetCursorPosY(y + padding);
  ImGui::SetCursorPosX(x + padding);
  ImGui::Text(name.data());

  ImGui::PopStyleColor();

  if (clicked)
    toggled = !toggled;
  return clicked;
}

} // namespace terra