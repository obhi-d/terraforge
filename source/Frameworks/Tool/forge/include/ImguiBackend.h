
#pragma once
#include "ImguiTheme.h"
#include "Setup.h"
#include <GfxDevice43.h>
#include <GlGfx.h>
#include <imgui/imgui.h>

namespace terra
{
enum class ImWith
{
  fMinimize = 1 << 0,
  fMaximize = 1 << 1,
  fRestore  = 1 << 2,
  fClose    = 1 << 3,
  fMenu     = 1 << 4,
  fLogo     = 1 << 5,
};

ENUM_FLAGS(ImWith);

enum class WindowAction
{
  eNone,
  eToggleSize,
  eMinimize,
  eMaximize,
  eRestore,
  eDrag,
  eClose
};

enum class ImAlign
{
  eRight,
  eLeft
};

class ImguiBackend
{
public:
  void init(std::shared_ptr<GfxDevice43>);
  void destroy();
  void draw();
  void applyTheme(ImguiTheme const&);

  // Draw Helpers
  ImAlign align(ImAlign align, float padding = 1.0f);
  // Draw a titlebar with flags, returns the button name clicked
  // if Menu flag is used, cursor is placed at next menu draw
  // You can right align to draw from right
  TitlebarAction beginTitlebar(ImVec2 size, ImWith flags);
  void       endTitlebar();
  void       textCentered(std::string_view text, ImVec2 pos, ImVec2 size);
  void       imageIcon(ImageName, ImVec2 size, Color tint);
  bool       iconButton(std::string_view name, ImVec2 size, Color color, Color hover);
  bool       drawResizeControl(glm::ivec2 windowSize);
  

private:
  void createDeviceObjects();
  void draw(glm::vec2 frameSize, ImDrawData*);
  void createBuffers(ImDrawData*);
  void uploadFonts(ImguiTheme const&);

  GlGfxState state;

  struct PackInfo
  {
    glm::uvec2 offset;
    glm::uvec2 size;
  };

  struct PackUV
  {
    ImVec2 uv0 = ImVec2(0, 0);
    ImVec2 uv1 = ImVec2(0, 0);
  };

  struct Params
  {
    glm::vec4 tint;
    glm::mat4 projection;
  };

  struct Extends
  {
    float left  = 0;
    float right = 0;
    float padding = 1.0f;
  };

  ImAlign alignment = ImAlign::eLeft;
  Extends currentRegExtends;

  ImThemeColors                            colors;
  Params                                   paramData;
  std::array<PackUV, ImagePackCount>       packUVs;
  std::array<GfxDescriptorSet::rhandle, 2> descriptors;
  GfxImage2D::handle                       font;
  GfxImage2D::handle                       image;
  GfxSampler::handle                       sampler;
  GfxDescriptorSetLayout::handle           descriptorSetLayout;
  GfxDescriptorSet::handle                 descriptorSet;
  GfxBuffer::handle                        vertexData;
  GfxBuffer::handle                        indexData;
  GfxBuffer::handle                        params;
  uint32_t                                 vertexDataSize    = 0;
  uint32_t                                 indexDataSize     = 0;
  GfxMesh::handle                          layout;
  GfxProgram::handle                       effect;
  glm::vec4                                clearColor;
  std::shared_ptr<GfxDevice43>             renderer;
  std::vector<GfxBuffer::handle>           pendingDeletion;
};
} // namespace terra