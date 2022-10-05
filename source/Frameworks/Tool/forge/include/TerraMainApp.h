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

private:
  ImguiTerraWindow       viewer;
  SDL_GLContext          glContext = nullptr;
  std::vector<glm::vec2> dpiScaling;
  AppSettings            settings;
  neo::registry          themeReader;
};
} // namespace terra
