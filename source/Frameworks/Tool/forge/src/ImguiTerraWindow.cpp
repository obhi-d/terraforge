
#include "GlGfx.h"
#include <Common.h>
#include <ImguiTerraWindow.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>

namespace terra
{

void ImguiTerraWindow::create(SDL_GLContext glContext, std::shared_ptr<GfxDevice43> device, AppSettings const& settings)
{
  this->size     = settings.viewerSize;
  this->position = settings.viewerPos;

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS;

  window = SDL_CreateWindow(settings.name.data(), position.x, position.y, size.x, size.y,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI );

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

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle& style                 = ImGui::GetStyle();
  style.WindowRounding              = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  backend.init(device);
  //nodeEditor.scanNodeMetas();
}

void ImguiTerraWindow::setTheme(ImguiTheme const& theme)
{
  backend.applyTheme(theme);
}

bool ImguiTerraWindow::pollEvents()
{
  auto&     io = ImGui::GetIO();
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {

    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      return false;
    switch (event.type)
    {
    case SDL_WINDOWEVENT:
    {
      switch (event.window.event)
      {
      case SDL_WINDOWEVENT_CLOSE:
        if (event.window.windowID == SDL_GetWindowID(window))
          return false;
        break;
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        size.x       = event.window.data1;
        size.y = event.window.data2;
        break;
      }
    }
    }
    /*
    switch (event.type)
    {
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT)
        mouseDragging = false;
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
        windowSize.x = event.window.data1;
        windowSize.y = event.window.data2;
        break;
      case SDL_WINDOWEVENT_MINIMIZED:
        if (event.window.windowID == SDL_GetWindowID(window))
        {
          savedState = state;
          state = eMinimized;
        }
        break;
      case SDL_WINDOWEVENT_MAXIMIZED:
        if (event.window.windowID == SDL_GetWindowID(window))
          state = eMaximized;

        break;
      case SDL_WINDOWEVENT_RESTORED:
        if (event.window.windowID == SDL_GetWindowID(window))
        {
          if (state == eMinimized)
          {
            if (savedState == eWindowed)
            {
              windowSize = size;
              SDL_SetWindowSize(window, size.x, size.y);
              SDL_SetWindowPosition(window, position.x, position.y);
              state = eWindowed;
            }
            
          }
          SDL_SetWindowInputFocus(window);
        }
        break;
      }
    }
    break;
    }
  */
  }

  return true;
}

void ImguiTerraWindow::drawWindowDecoration()
{
  /* auto& io    = ImGui::GetIO();
  auto  flags = ImWith::fClose | ImWith::fMenu | ImWith::fMinimize | ImWith::fLogo | ImWith::fResizeCtrl;
  if (state == eMaximized)
    flags = flags | ImWith::fRestore;
  else
    flags = flags | ImWith::fMaximize;
  
  glm::ivec2 start, size;
  SDL_GetWindowPosition(window, &start.x, &start.y);
  size.x = (int)io.DisplaySize.x;
  size.y = (int)io.DisplaySize.y;
  backend.setRegion(start, size);
  switch (backend.windowDecoration(flags))
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
  }
  break;
  case WindowAction::eRestore:
  {
    SDL_RestoreWindow(window);
  }
  break;
  case WindowAction::eMinimize:
  {
    SDL_GetWindowSize(window, &size.x, &size.y);
    SDL_GetWindowPosition(window, &position.x, &position.y);
    SDL_MinimizeWindow(window);
  }
  break;
  case WindowAction::eDrag:
  {
    if (!windowDragging)
    {
      SDL_GetWindowPosition(window, &position.x, &position.y);
      SDL_GetGlobalMouseState(&mouseLast.x, &mouseLast.y);
      mouseDragging = windowDragging = true;
    }
  }
  break;
  case WindowAction::eToggleSize:
  {
    if (state == eMaximized)
      SDL_RestoreWindow(window);
    else
      SDL_MaximizeWindow(window);
  }
  break;
  case WindowAction::eResize:
    if (!windowResizing)
    {
      SDL_GetWindowSize(window, &this->size.x, &this->size.y);
      SDL_GetGlobalMouseState(&mouseLast.x, &mouseLast.y);
      mouseLatest   = mouseLast;
      mouseDragging = windowResizing = true;
    }
    break;
  }*/
}

void ImguiTerraWindow::draw()
{
  auto& io = ImGui::GetIO();
  assert(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  //drawWindowDecoration();
  //drawResizeControl();
  nodeEditor.drawNodeEditor(backend);
  // Rendering
  ImGui::Render();
  backend.draw();
  SDL_GL_SwapWindow(window);
  // pending evvents
  /*
  if (windowDragging)
  {
    SDL_GetGlobalMouseState(&mouseLatest.x, &mouseLatest.y);
    auto delta = mouseLatest - mouseLast;
    SDL_SetWindowPosition(window, (int)(position.x + delta.x), (int)(position.y + delta.y));
    windowDragging = mouseDragging;
  }
  if (windowResizing)
  {
    SDL_GetGlobalMouseState(&mouseLatest.x, &mouseLatest.y);
    auto delta = mouseLatest - mouseLast;
    SDL_SetWindowSize(window, size.x + delta.x, size.y + delta.y);
    windowResizing = mouseDragging;
  }*/
  
}
} // namespace terra