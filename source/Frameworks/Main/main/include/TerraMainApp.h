#include <optional>


#include <Magnum/Platform/GLContext.h>
#include <imgui/ImguiTerraContext.h>

using namespace Magnum;
namespace terra
{
    struct AppSettings
    {
        bool hasGPGPU = false;
        bool verbose  = false;
    };

    class TerraMainApp
    {
    public:
        TerraMainApp();
        ~TerraMainApp();

        void       initalize();
        void       createContext();
        void       updateMonitorScaling();
        void       run();
        void       draw();
        static int Main( int argc, const char* argv[] );

    private:
        std::optional<Platform::GLContext> context;
        SDL_GLContext                      glContext = nullptr;
        std::vector<Vector2>               dpiScaling;
        AppSettings                        settings;
        ImguiTerraContext                  imguiContext;
    };
} // namespace terra
