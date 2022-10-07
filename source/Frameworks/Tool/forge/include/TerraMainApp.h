#include "GfxDevice43.h"
#include "NeoHelper.h"
#include "Setup.h"
#include <optional>
#include <string>

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
  void       addString(std::string name, std::u8string value)
  {
    stringTable[name] = value;
  }

  void readLocalization();

private:
  std::unordered_map<std::string, std::u8string> stringTable;
  std::shared_ptr<GfxDevice43>                     device;
  ImguiTerraWindow                               viewer;
  SDL_GLContext                                  glContext = nullptr;
  std::vector<glm::vec2>                         dpiScaling;
  AppSettings                                    settings;
  neo::registry                                  themeReader;
};
} // namespace terra
