
#pragma once
#include "ImguiTheme.h"
#include "Setup.h"
#include <GfxDevice43.h>
#include <GlGfx.h>
#include <imgui.h>
#include <tuple>

namespace terra
{

enum class ImAlign
{
  eRight,
  eLeft
};

class ImguiTerraWindow;
class ImguiBackend
{
public:
  struct CallbackData
  {
    void* instance;
    Rect viewport;
    Rect scissor;
  };

  void init(std::shared_ptr<GfxDevice43>);
  void destroy();
  void draw();
  void drawOtherWindows();
  void applyTheme(ImguiTheme const&);

  // Draw Helpers
  void    setRegion(glm::ivec2 start, glm::ivec2 size);
  ImAlign setLayout(glm::ivec2 start, glm::ivec2 size, ImAlign align, float padding = 1.0f);
  ImAlign align(ImAlign);
  // Draw a titlebar with flags, returns the button name clicked
  // if Menu flag is used, cursor is placed at next menu draw
  // You can right align to draw from right
  void         endTitlebar();
  void         textCentered(std::string_view text, ImVec2 pos, ImVec2 size);
  bool         iconButton(std::string_view name, ImVec2 size, Color color, Color hover);
  bool         drawResizeControl(glm::ivec2 windowSize);
  bool iconButton(char16_t, glm::ivec2 size, int iconSize, Color normal, Color hover, Color pressed, bool inlay = true);
  bool iconButton(ImageName, glm::ivec2 size, int iconSize, Color normal, Color hover, Color pressed,
                  bool inlay = true);
  bool toggleButton(std::string_view name, bool& toggled, ImVec2 size, std::u8string_view tip, float padding = 0.0f);

private:
  std::tuple<bool, glm::ivec2, Color> iconButtonSetup(glm::ivec2 size, int iconSize, bool inlay, Color normal,
                                                      Color hover, Color pressed);
  bool                                isIntersecting();

  static bool isIntersecting(glm::ivec2 mouse, glm::ivec2 pos, glm::ivec2 size)
  {
    return (pos.x <= mouse.x && mouse.x <= pos.x + size.x) && (pos.y <= mouse.y && mouse.y <= pos.y + size.y);
  }
  void drawIcon(ImageName, glm::ivec2 location, glm::ivec2 size, Color color);
  void drawIcon(char16_t, glm::ivec2 location, glm::ivec2 size, Color color);
  void pushQuad(glm::ivec2 location, glm::ivec2 size, glm::vec2 uv0, glm::vec2 uv1, Color color = 0xffffffff);

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
    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(0, 0);
  };

  struct Params
  {
    glm::vec4 tint;
    glm::mat4 projection;
  };

  struct Extends
  {
    glm::ivec2 min;
    glm::ivec2 max;
    int        dx() const
    {
      return max.x - min.x;
    }
    int dy() const
    {
      return max.y - min.y;
    }
    float padding = 1.0f;
  };

  ImAlign alignment = ImAlign::eLeft;
  Extends currentRegExtends;

  glm::vec2                                whiteUV;
  std::vector<ImDrawVert>                  internalDrawVtx;
  std::vector<ImDrawIdx>                   internalDrawIdx;
  ImguiTheme const*                        theme = nullptr;
  Params                                   paramData;
  std::array<PackUV, ImagePackCount>       packUVs;
  std::array<GfxDescriptorSet::rhandle, 2> descriptors;
  GfxImage2D::handle                       font;
  GfxSampler::handle                       sampler;
  GfxDescriptorSetLayout::handle           descriptorSetLayout;
  GfxDescriptorSet::handle                 descriptorSet;
  GfxBuffer::handle                        vertexData;
  GfxBuffer::handle                        indexData;
  GfxBuffer::handle                        params;
  uint32_t                                 vertexDataSize = 0;
  uint32_t                                 indexDataSize  = 0;
  GfxMesh::handle                          layout;
  GfxProgram::handle                       effect;
  glm::vec4                                clearColor;
  std::shared_ptr<GfxDevice43>             renderer;
  std::vector<GfxBuffer::handle>           pendingDeletion;
};
} // namespace terra