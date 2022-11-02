#include "GfxDevice43.h"
#include "NeoHelper.h"
#include "Setup.h"
#include <optional>
#include <string>

#include "ImguiTheme.h"
#include <ImguiTerraWindow.h>

namespace terra
{

class TerraMainApp
{
public:
  TerraMainApp();
  ~TerraMainApp();

  void readSettings() {}
  void writeSettings() {}
  void initalize();
  void createContext();
  void updateMonitorScaling();
  void run();
  void draw();
  void reloadTheme();

  static int Main(int argc, const char* argv[]);

  void addString(std::string name, std::u8string value)
  {
    stringTable[name] = value;
  }

  std::u8string_view getLocalizedString(std::string_view name)
  {
    auto it = stringTable.find(std::string(name));
    if (it != stringTable.end())
      return it->second;
    logError("Cound not find string entry for: {}", name);
    auto iit = stringTable.emplace(name, std::u8string((char8_t*)name.data(), (char8_t*)name.data() + name.length()));
    return iit.first->second;
  }

  void readLocalization();

  SDL_GLContext getGlContext() const
  {
    return glContext;
  }

  std::shared_ptr<GfxDevice43> const& getDevice() const
  {
    return device;
  }

  AppSettings const getSettings() const
  {
    return settings;
  }

  auto const& getTheme() const
  {
    return theme;
  }

  private:
  std::unordered_map<std::string, std::u8string> stringTable;
  std::shared_ptr<GfxDevice43>                   device;
  ImguiTerraWindow                               viewer;
  SDL_GLContext                                  glContext = nullptr;
  std::vector<glm::vec2>                         dpiScaling;
  AppSettings                                    settings;
  neo::registry                                  themeReader;
  ImguiTheme                                     theme;
  uint32_t                                       frame = 0;
};
} // namespace terra
