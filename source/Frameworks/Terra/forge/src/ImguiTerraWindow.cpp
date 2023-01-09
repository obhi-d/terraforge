
#include "DrawHelpers.h"
#include "GlGfx.h"
#include "TerraMainApp.h"
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

  ImGuiIO& io                          = ImGui::GetIO();
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
  settingName                   = app.localize("Settings");
  mainWindowName                = app.localize("Forge");
  meshDrawData.instance         = &app;
  previewWindow.canBeMaximized  = true;
  previewWindow.isMain          = false;
  previewWindow.locked          = false;
  previewWindow.maximized       = false;
  settingsWindow.canBeMaximized = false;
  settingsWindow.isMain         = false;
  settingsWindow.locked         = false;
  settingsWindow.maximized      = false;
}

void ImguiTerraWindow::deinit(TerraMainApp& app)
{
  meshPreview.deinit(app);
  nodeEditor.deinit(app);
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
      mouseState.mouseDelta.x += event.motion.x - mouseState.mousePosition.x;
      mouseState.mouseDelta.y += event.motion.y - mouseState.mousePosition.y;
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
    case SDL_QUIT:
      // quit event
      return false;
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
  bool  regenerate = false;
  if (settingsWindow.opened)
  {
    setHeaderFont();
    ImGui::SetNextWindowSizeConstraints(ImVec2(40, 40), ImVec2(10000, 10000));
    if (ImGui::Begin((char const*)settingName.data(), nullptr, settingsWindow.locked ? ImGuiWindowFlags_NoMove : 0))
    {
      switch (drawTitleMenu(settingsWindow))
      {
      case WindowAction::eClose:
        settingsWindow.opened = false;
        break;
      }
      setNormalFont();
      // Settings
      float item_height = ImGui::GetTextLineHeightWithSpacing();
      {
        static std::u8string_view header = app.localize("genParams");
        ImGui::Text("%s", (const char*)header.data());
        ImGui::Separator();
        regenerate |= drawProp(app, settings.frequency, 0, std::numeric_limits<float>::max(), 0.001f);
        regenerate |= drawProp(app, settings.seed, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
      }
      {
        static std::u8string_view header = app.localize("exportParams");
        ImGui::Text("%s", (const char*)header.data());
        ImGui::Separator();
        regenerate |= drawProp(app, settings.tileSize, 4, 8129);
        regenerate |= drawProp(app, settings.tileOffset, 1, std::numeric_limits<int>::max());
        regenerate |= drawProp(app, settings.previewTile, 0, std::numeric_limits<int>::max());
      }
      {
        static std::u8string_view header = app.localize("previewParams");
        ImGui::Text("%s", (const char*)header.data());
        ImGui::Separator();
        drawProp(app, meshPreview.sunColor);
        drawProp(app, meshPreview.sunIntensity, 0.01f, std::numeric_limits<float>::max(), 0.05f);
        drawProp(app, meshPreview.meshTint);
        drawProp(app, meshPreview.water);
        drawProp(app, meshPreview.vegetation);
        drawProp(app, meshPreview.rocks);
        drawProp(app, meshPreview.terrain);
        drawProp(app, meshPreview.layerWeights, 0.0f, 1.0f, .01f);
        // drawProp(app, meshPreview.shadowMapResolution, 0, 3);
        // drawProp(app, meshPreview.planetScale, 0.0f, std::numeric_limits<float>::max(), 0.5f);
        drawProp(app, meshPreview.heightScale, 0.0f, std::numeric_limits<float>::max(), 0.5f);
        drawProp(app, meshPreview.showWaterLevel);
        drawProp(app, meshPreview.waveLevel, 1.0f, -1.0f, 0.005f);
        drawProp(app, meshPreview.wavePeriod, 0.0f, 21.f, 0.5f);
      }
      {
        static std::u8string_view header = app.localize("camera");
        ImGui::Text("%s", (const char*)header.data());
        ImGui::Separator();
        auto& camera = meshPreview.getCamera();
        drawProp(app, camera.ortho);
        drawProp(app, camera.fov, 0, 180, .5f);
        drawProp(app, camera.scrollSpeed, 0.01f, 1000000.f, .5f);
        drawProp(app, camera.distanceFactor, 0.01f, 1000000.f, .5f);
      }
      {
        static std::u8string_view header = app.localize("nodeParams");
        ImGui::Text("%s", (const char*)header.data());
        ImGui::Separator();
        nodeEditor.drawNodeSettings(app, backend);
      }
      popNormalFont();
    }
    ImGui::End();
    popHeaderFont();
  }
  if (regenerate)
  {
    // validate settings
    settings.tileSize.get().x = std::clamp<uint32_t>(settings.tileSize.get().x, 2, 8129);
    settings.tileSize.get().y = std::clamp<uint32_t>(settings.tileSize.get().y, 2, 8129);
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

  if (previewWindow.opened)
  {
    ImGui::SetNextWindowSizeConstraints(ImVec2(256, 256), ImVec2(10000, 10000));
    setHeaderFont();
    if (ImGui::Begin((const char*)mainWindowName.data(), nullptr, previewWindow.locked ? ImGuiWindowFlags_NoMove : 0))
    {
      switch (drawTitleMenu(previewWindow))
      {
      case WindowAction::eRestore:
        ImGui::RestoreWindow();
        break;
      case WindowAction::eMaximize:
        ImGui::MaximizeWindow();
        break;
      case WindowAction::eClose:
        previewWindow.opened = false;
        break;
      }
      setNormalFont();
      ImGui::GetWindowDrawList()->AddCallback(
        [](const ImDrawList* parent_list, const ImDrawCmd* cmd)
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
      popNormalFont();
    }

    ImGui::End();
    popHeaderFont();
  }

  ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  if (!nodeEditor.drawNodeEditor(app, backend))
  {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
  }
  drawSettings(app);
  // Rendering
  ImGui::Render();

  backend.draw();
  SDL_GL_SwapWindow(window);
  backend.drawOtherWindows();
  return true;
}

void ImguiTerraWindow::tick()
{
  meshPreview.tick();
  nodeEditor.tick();
}

} // namespace terra
