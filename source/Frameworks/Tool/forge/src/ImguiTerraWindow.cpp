
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

  window = SDL_CreateWindow(settings.name.data(), position->x, position->y, size->x, size->y, flags);

  if (!window)
    throw std::runtime_error("Could not create window!");
  windowID = SDL_GetWindowID(window);
  SDL_SetWindowResizable(window, SDL_TRUE);
  SDL_GL_MakeCurrent(window, glContext);

  imguiContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(imguiContext);

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
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
      mouseState.mousePosition.x = event.motion.x;
      mouseState.mousePosition.y = event.motion.y;
      mouseState.mouseDelta.x    = event.motion.xrel;
      mouseState.mouseDelta.y    = event.motion.yrel;
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

  return true;
}

void ImguiTerraWindow::drawWindowDecoration()
{
  auto& io    = ImGui::GetIO();
  auto  flags = ImWith::fClose | ImWith::fMenu | ImWith::fMinimize | ImWith::fLogo | ImWith::fResizeCtrl;

  if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
    flags = flags | ImWith::fRestore;
  else
    flags = flags | ImWith::fMaximize;

  glm::ivec2 start, size;
  SDL_GetWindowPosition(window, &start.x, &start.y);
  size.x = (int)io.DisplaySize.x;
  size.y = (int)io.DisplaySize.y;
  backend.setRegion(start, size);
  switch (backend.windowDecoration(*this, flags))
  {
  case WindowAction::eClose:
  {
    SDL_Event quit;
    quit.type = SDL_QUIT;
    SDL_PushEvent(&quit);
  }
  case WindowAction::eMaximize:
  {
    SDL_MaximizeWindow(window);
    SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
  }
  break;
  case WindowAction::eRestore:
  {
    SDL_RestoreWindow(window);
    SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
  }
  break;
  case WindowAction::eMinimize:
  {
    SDL_MinimizeWindow(window);
  }
  break;
  case WindowAction::eDrag:
  {
    if (!windowDragging)
    {
      SDL_GetWindowSize(window, &dragData.startSize.x, &dragData.startSize.y);
      SDL_GetWindowPosition(window, &dragData.startPosition.x, &dragData.startPosition.y);
      SDL_GetGlobalMouseState(&dragData.mouse.x, &dragData.mouse.y);
      windowDragging    = true;
      mouseState.locked = MouseLockedBy::eMainWndDecorations;
    }
  }
  break;
  case WindowAction::eToggleSize:
  {
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
      SDL_RestoreWindow(window);
    else
      SDL_MaximizeWindow(window);
  }
  break;
  case WindowAction::eResize:
    if (!windowResizing)
    {
      SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
      dragData.startSize = windowSize;
      SDL_GetWindowPosition(window, &dragData.startPosition.x, &dragData.startPosition.y);
      SDL_GetGlobalMouseState(&dragData.mouse.x, &dragData.mouse.y);
      windowResizing    = true;
      mouseState.locked = MouseLockedBy::eMainWndDecorations;
    }
    break;
  }
}

void ImguiTerraWindow::draw(TerraMainApp& app)
{
  SDL_GL_MakeCurrent(window, app.getGlContext());

  auto& io = ImGui::GetIO();
  assert(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  if (io.WantCaptureMouse)
    mouseState.mainWnd = false;
  app.getDevice()->flushStates();
  GlGfxState state;
  state.viewport.offset = glm::ivec2(0, 0);
  state.viewport.size   = windowSize;
  app.getDevice()->setState(state);
  app.getDevice()->clearBackbuffer(app.getTheme().themeColors.clear, true);
  drawWindowDecoration();
  // drawResizeControl();
  nodeEditor.drawNodeEditor(app, backend);
  // Rendering
  ImGui::Render();

  meshPreview.update(windowSize, mouseState);
  meshPreview.draw(windowSize, app);
  backend.draw();
  SDL_GL_SwapWindow(window);
  backend.drawOtherWindows();
  // pending evvents

  if (windowDragging)
  {
    glm::ivec2 mouseLatest;
    SDL_GetGlobalMouseState(&mouseLatest.x, &mouseLatest.y);
    auto delta = mouseLatest - dragData.mouse;
    SDL_SetWindowPosition(window, (int)(dragData.startPosition.x + delta.x), (int)(dragData.startPosition.y + delta.y));
    windowDragging = mouseState.leftDown && mouseState.dragging;
  }
  else if (windowResizing)
  {
    glm::ivec2 mouseLatest;
    SDL_GetGlobalMouseState(&mouseLatest.x, &mouseLatest.y);
    auto delta = mouseLatest - dragData.mouse;
    windowSize = glm::ivec2{dragData.startSize.x + delta.x, dragData.startSize.y + delta.y};
    SDL_SetWindowSize(window, windowSize.x, windowSize.y);
    windowResizing = mouseState.leftDown && mouseState.dragging;
  }
}
} // namespace terra