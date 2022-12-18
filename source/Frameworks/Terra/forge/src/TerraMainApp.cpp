#define NEO_HEADER_ONLY_IMPL
#include <SDL.h>
#include <TerraMainApp.h>

#include <iostream>
#include <stdexcept>

#include "DrawHelpers.h"
#include "GfxDevice45.h"
#include "GlGfx.h"
#include "ImageSerializer.h"
#include "ImguiTheme.h"
#include "NeoHelper.h"
#include "ResourceUtils.h"
#include "Terra.h"

neo_registry(ThemeBuilder);
neo_registry(StringBuilder);

namespace terra
{
std::unique_ptr<TerraMainApp> g_app;

TerraMainApp::TerraMainApp()
{
  get().addImageCodec(".png", std::make_shared<ImageSerializer>());
  readSettings();
  if (settings.verbose)
    Logger::get().open(Logger::Debug);
  else
    Logger::get().open(Logger::Info);
}

TerraMainApp::~TerraMainApp() {}

void TerraMainApp::destroy()
{
  viewer.deinit(*this);
  get().destroy();
  device = nullptr;
  SDL_Quit();
}

void TerraMainApp::readLocalization()
{
  auto bytes = fileContentToBytes("localization/" + settings.language.get() + ".nls");

  std::u8string_view ss((char8_t const*)bytes.data(), bytes.size());
  while (!ss.empty())
  {
    auto nameStart = ss.find_first_not_of(u8" \t\n\r");
    if (nameStart == ss.npos)
      break;
    auto nameEnd = ss.find_first_of(u8" \t=\n\r", nameStart + 1);
    if (nameEnd == ss.npos)
      break;
    auto name     = ss.substr(nameStart, nameEnd - nameStart);
    ss            = ss.substr(nameEnd);
    auto valStart = ss.find(u8"\"\"\"");
    if (valStart == ss.npos)
      break;
    valStart += 3;
    auto valEnd = ss.find(u8"\"\"\";", valStart);
    if (valEnd == ss.npos)
      break;
    auto value = ss.substr(valStart, valEnd - valStart);
    ss         = ss.substr(valEnd + 4);

    // format is name =
    // """...
    // ...""";
    addString(std::string((char const*)name.data(), (char const*)name.data() + name.length()), std::u8string(value));
  }
}

void TerraMainApp::initalize()
{
  if (SDL_Init(SDL_INIT_EVENTS))
  {
    throw std::runtime_error("SDL init failed.");
  }
  ThemeRegister(ThemeBuilder, themeReader);
  readLocalization();
  scanScripts();
}

void TerraMainApp::scanScripts()
{
  auto const& path    = getMediaPath();
  auto        effects = path / "scripts";
  if (std::filesystem::exists(effects))
  {
    for (auto const& dir_entry : std::filesystem::directory_iterator{effects})
    {
      auto filepath = dir_entry.path();
      if (dir_entry.is_regular_file() && filepath.extension() == "gfx")
        get().scanShader(filepath);
    }
  }
}

void TerraMainApp::reloadTheme()
{
  neo::state_machine sm{themeReader, &theme};
  auto               f1_str = fileContentToString(settings.theme);
  sm.parse(settings.theme.get(), f1_str);
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

  auto constexpr DebugActive = SDL_GL_CONTEXT_DEBUG_FLAG;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, DebugActive | SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG);

  glContext   = SDL_GL_CreateContext(window);
  int version = 450;
  if (!glContext)
  {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    glContext = SDL_GL_CreateContext(window);

    if (!glContext)
    {
      // try without robust access
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, DebugActive);
      glContext                = SDL_GL_CreateContext(window);
      settings.hasRobustAccess = false;
      if (!glContext)
        throw std::runtime_error("createContext(): cannot create glContext");
    }
    version = 430;
  }

  tgl::initialize(reinterpret_cast<glb::ProcAddress (*)(const char*)>(SDL_GL_GetProcAddress), false);
  if (version == 430)
    device = std::make_shared<GfxDevice43>();
  else
    device = std::make_shared<GfxDevice45>();
}

void TerraMainApp::run()
{
  initalize();
  createContext();
  terra::get().init(
    [this](std::string_view& name) -> std::u8string_view
    {
      return localizeNamed(name);
    },
    device);

  viewer.init(*this);
  reloadTheme();

  do
  {
    if (!viewer.pollEvents())
      return;
    if (!draw())
      return;
    events.dispatch(*this);
  }
  while (true);
}

int TerraMainApp::Main(int argc, const char* argv[])
{
  g_app.reset(new TerraMainApp());
#ifdef NDEBUG
  try
#endif
  {
    g_app->run();
  }
#ifdef NDEBUG
  catch (std::exception& ex)
  {
    logError(ex.what());
    std::cerr << ex.what();
    std::exit(-1);
  }
#endif
  g_app->destroy();
  return 0;
}

bool TerraMainApp::draw()
{
  bool r = viewer.draw(*this);
  frame++;
  return r;
}

TerraMainApp& app()
{
  return *g_app.get();
}

} // namespace terra

int main(int argc, char** argv)
{
  return terra::TerraMainApp::Main(argc, (const char**)argv);
}
