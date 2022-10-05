#define NEO_HEADER_ONLY_IMPL
#include <SDL.h>
#include <TerraMainApp.h>

#include <iostream>
#include <stdexcept>

#include "GlGfx.h"
#include "ImguiTheme.h"
#include "ResourceUtils.h"

neo_registry(ThemeBuilder);

namespace terra
{
TerraMainApp::TerraMainApp()
{
  // readSettings();
  if (settings.verbose)
    Logger::get().open(Logger::Debug);
  else
    Logger::get().open(Logger::Info);
}

TerraMainApp::~TerraMainApp()
{
  SDL_Quit();
}

void TerraMainApp::initalize()
{
  if (SDL_Init(SDL_INIT_EVENTS))
  {
    throw std::runtime_error("SDL init failed.");
  }
  ThemeRegister(ThemeBuilder, themeReader);
  reloadTheme();
}

void TerraMainApp::reloadTheme()
{
  ImguiTheme         theme;
  neo::state_machine sm{themeReader, &theme};
  auto               f1_str = fileContentToString(settings.theme);
  sm.parse(settings.theme, f1_str);
  if (!sm.fail_bit())
  {
    viewer.setTheme(theme);
  }
}

void TerraMainApp::updateMonitorScaling()
{
  int displays = SDL_GetNumVideoDisplays();
  dpiScaling.resize(displays);
  for (int i = 0; i < displays; ++i)
  {
    glm::vec2 dpi;
    if (SDL_GetDisplayDPI(i, nullptr, &dpi.x, &dpi.y) == 0)
    {
      glm::vec2 scaling{dpi / 96.0f};
      logInfo("Virtual DPI scaling {}.{}", scaling.x, scaling.y);
      dpiScaling[i] = dpi;
    }
  }
}

void TerraMainApp::createContext()
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

  SDL_Window* window = nullptr;
  if (!(window =
          SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 16, 16,
                           SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN)))
  {
    throw std::runtime_error("createContext(): cannot create window");
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  glContext = SDL_GL_CreateContext(window);
  if (!glContext)
  {
    throw std::runtime_error("createContext(): cannot create glContext");
  }

  tgl::initialize(reinterpret_cast<glb::ProcAddress (*)(const char*)>(SDL_GL_GetProcAddress), false);
}

void TerraMainApp::run()
{
  initalize();
  createContext();
  viewer.create(glContext, settings);
  do
  {
    if (!viewer.pollEvents())
      return;
    draw();
  }
  while (true);
}

int TerraMainApp::Main(int argc, const char* argv[])
{
  TerraMainApp app;
  try
  {
    app.run();
  }
  catch (std::exception& ex)
  {
    std::cerr << ex.what();
    std::exit(-1);
  }
  return 0;
}

void TerraMainApp::draw()
{
  viewer.draw();
}

} // namespace terra

int main(int argc, char** argv)
{
  return terra::TerraMainApp::Main(argc, (const char**)argv);
}
