
#include "GlGfx.h"
#include <Common.h>
#include <ImguiTerraWindow.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

namespace terra
{

void ImguiTerraWindow::create(SDL_GLContext glContext, AppSettings const& settings)
{
  this->size     = settings.viewerSize;
  this->position = settings.viewerPos;

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS;

  window = SDL_CreateWindow(settings.name.data(), position.x, position.y, size.x, size.y,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);

  if (window)
    windowID = SDL_GetWindowID(window);
  SDL_GL_MakeCurrent(window, glContext);
 
  imguiContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(imguiContext);

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

  std::string glslVersion = "#version ";
  glslVersion += std::to_string(settings.glslVersion);
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle& style                 = ImGui::GetStyle();
  style.WindowRounding              = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  ImGui_ImplOpenGL3_Init(glslVersion.c_str());
}

void ImguiTerraWindow::setTheme(ImguiTheme const&) {}

bool ImguiTerraWindow::pollEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      return false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(window))
      return false;
  }
  return true;
}

void ImguiTerraWindow::draw()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  auto& io = ImGui::GetIO();
  // Rendering
  ImGui::Render();
  gl::glViewport(0, 0, (gl::GLsizei)io.DisplaySize.x, (gl::GLsizei)io.DisplaySize.y);
  gl::glClearColor(0, 0, 0, 1);
  gl::glClear(gl::GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_Window*   backup_current_window  = SDL_GL_GetCurrentWindow();
  SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();
  SDL_GL_MakeCurrent(backup_current_window, backup_current_context);

  SDL_GL_SwapWindow(window);
 
}
} // namespace terra