
#include "GlGfx.h"
#include "TerraMainApp.h"
#include "DrawHelpers.h"
#include <Common.h>
#include <ImguiTerraWindow.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>

namespace terra
{

void ImguiTerraWindow::init(TerraMainApp& app)
{
  SDL_GLContext                glContext = app.getGlContext();
  std::shared_ptr<GfxDevice43> device    = app.getDevice();
  AppSettings const&           settings  = app.getSettings();

  auto size     = settings.viewerSize;
  auto position = settings.viewerPos;

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;

  SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
  SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");

  window = SDL_CreateWindow(settings.name.data(), position.x, position.y, size.x, size.y, flags);

  if (!window)
    throw std::runtime_error("Could not create window!");
  windowID = SDL_GetWindowID(window);
  SDL_SetWindowResizable(window, SDL_TRUE);
  SDL_GL_MakeCurrent(window, glContext);

  imguiContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(imguiContext);

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
  io.HoverDelayNormal = 2.2f;
  io.HoverDelayShort  = 0.5f;
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle& style                 = ImGui::GetStyle();
  style.WindowRounding              = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  backend.init(device);
  nodeEditor.init(app);
  meshPreview.init(app);
  SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
  settingName = app.getLocalizedString("@Settings");
  mainWindowName        = app.getLocalizedString("@Forge");
  meshDrawData.instance = &app;
}

void ImguiTerraWindow::setTheme(ImguiTheme const& theme)
{
  backend.applyTheme(theme);
}

bool ImguiTerraWindow::pollEvents()
{
  mouseState.middleDelta  = 0.0f;
  mouseState.mouseDelta.x = 0;
  mouseState.mouseDelta.y = 0;
  auto&     io            = ImGui::GetIO();
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {

    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      return false;

    switch (event.type)
    {
    case SDL_MOUSEMOTION:
      mouseState.mouseDelta.x    += event.motion.x - mouseState.mousePosition.x;
      mouseState.mouseDelta.y    += event.motion.y - mouseState.mousePosition.y;
      mouseState.mousePosition.x = event.motion.x;
      mouseState.mousePosition.y = event.motion.y;
      mouseState.dragging        = mouseState.leftDown || mouseState.rightDown;
      mouseState.mainWnd         = event.motion.windowID == this->windowID;
      break;
    case SDL_MOUSEWHEEL:
      mouseState.middleDelta = event.wheel.preciseY;
      mouseState.mainWnd     = event.wheel.windowID == this->windowID;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT)
        mouseState.leftDown = true;
      else if (event.button.button == SDL_BUTTON_RIGHT)
        mouseState.rightDown = true;
      break;
    case SDL_MOUSEBUTTONUP:
      mouseState.dragging = false;
      mouseState.locked   = MouseLockedBy::eNone;
      if (event.button.button == SDL_BUTTON_LEFT)
        mouseState.leftDown = false;
      else if (event.button.button == SDL_BUTTON_RIGHT)
        mouseState.rightDown = false;
      break;
    case SDL_WINDOWEVENT:
    {
      switch (event.window.event)
      {
      case SDL_WINDOWEVENT_CLOSE:
        if (event.window.windowID == SDL_GetWindowID(window))
          return false;
        break;
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        break;
      case SDL_WINDOWEVENT_MINIMIZED:
        break;
      case SDL_WINDOWEVENT_MAXIMIZED:
        break;
      case SDL_WINDOWEVENT_RESTORED:
        break;
      }
    }
    break;
    }
  }
  glm::ivec2 mouseLatest;
  SDL_GetGlobalMouseState(&mouseLatest.x, &mouseLatest.y);
  if (mouseLatest.x != mouseState.mousePosition.x || mouseLatest.y != mouseState.mousePosition.y)
    io.AddMousePosEvent((float)mouseLatest.x, (float)mouseLatest.y);
  return true;
}

void ImguiTerraWindow::drawSettings(TerraMainApp& app) 
{
  auto& settings   = app.getSettings();
  bool regenerate = false;
  if (ImGui::Begin((char const*)settingName.data()))
  {    
    // Settings
    float item_height = ImGui::GetTextLineHeightWithSpacing();
    if (ImGui::BeginChildFrame(ImGui::GetID("gen_settings"), ImVec2(-FLT_MIN, 6.25f * item_height), ImGuiWindowFlags_NoBackground))
    {
      static std::u8string_view header = app.getLocalizedString("@genParams");
      ImGui::Text("%s", (const char*)header.data());
      ImGui::Separator();
      regenerate |= drawProp(app, settings.frequency, 0, std::numeric_limits<float>::max(), 0.01f);
      regenerate |= drawProp(app, settings.seed, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
      ImGui::EndChildFrame();
    }
    if (ImGui::BeginChildFrame(ImGui::GetID("export_settings"), ImVec2(-FLT_MIN, 6.25f * item_height),
                               ImGuiWindowFlags_NoBackground))
    {
      static std::u8string_view header = app.getLocalizedString("@exportParams");
      ImGui::Text("%s", (const char*)header.data());
      ImGui::Separator();
      regenerate |= drawProp(app, settings.tileSize, 4, 8129);
      regenerate |= drawProp(app, settings.tileOffset, 1, std::numeric_limits<int>::max());
      regenerate |= drawProp(app, settings.nbPreviewTiles, 1, 8);
      ImGui::EndChildFrame();
    }
    if (ImGui::BeginChildFrame(ImGui::GetID("preview_settings"), ImVec2(-FLT_MIN, 12 * item_height),
                               ImGuiWindowFlags_NoBackground))
    {
      static std::u8string_view header = app.getLocalizedString("@previewParams");
      ImGui::Text("%s", (const char*)header.data());
      ImGui::Separator();
      drawProp(app, meshPreview.sunColor);
      drawProp(app, meshPreview.sunIntensity, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), 0.05f);
      drawProp(app, meshPreview.meshTint);
      drawProp(app, meshPreview.heightMultiplier, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), 0.05f);
      drawProp(app, meshPreview.heightTexPath);
      drawProp(app, meshPreview.meshStyle, 1.0f, std::numeric_limits<float>::max(), 0.5f);
      ImGui::EndChildFrame();
    }
    
  }
  ImGui::End();
  if (regenerate)
  {
    // validate settings
    settings.tileSize.get().x = std::clamp(settings.tileSize.get().x, 2, 8129);
    settings.tileSize.get().y = std::clamp(settings.tileSize.get().y, 2, 8129);
    app.regenWithActor({});
  }
}

bool ImguiTerraWindow::draw(TerraMainApp& app)
{
  static bool firstFrame = true;
  if (!firstFrame)
    SDL_HideWindow(window);

  firstFrame = false;
  // SDL_GL_MakeCurrent(window, app.getGlContext());

  auto& io = ImGui::GetIO();
  assert(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  app.getDevice()->flushStates();

  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
  
  if (!ImGui::Begin((const char*)mainWindowName.data(), &open, 0))
  {
    ImGui::End();
    SDL_DestroyWindow(window);
    return false;
  }

  if (!open)
  {
    SDL_DestroyWindow(window);
    return false;
  }

  ImGui::GetWindowDrawList()->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) 
    {
      auto& cbk  = *(ImguiBackend::CallbackData*)cmd->UserCallbackData;
      auto& app  = *(TerraMainApp*)cbk.instance;
      auto& self = (ImguiTerraWindow&)app.getWindow();

      self.meshPreview.update(cbk.scissor.size, self.mouseState);
      self.meshPreview.draw(cbk.viewport, cbk.scissor, app);
    },
    &meshDrawData);
  
  ImGui::InvisibleButton("main_viewer_trap", ImGui::GetContentRegionAvail());
  mouseState.mainWnd = ImGui::IsItemHovered();
  ImGui::End();
  ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  nodeEditor.drawNodeEditor(app, backend);
  drawSettings(app);
  // Rendering
  ImGui::Render();
  
  backend.draw();
  SDL_GL_SwapWindow(window);
  backend.drawOtherWindows();
  return true;
}
} // namespace terra
