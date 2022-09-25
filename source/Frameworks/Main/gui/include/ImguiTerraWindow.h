#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Timeline.h>

struct ImGuiContext;

using namespace Magnum;
namespace terra
{
    class ImguiTerraContext;
    class ImguiTerraWindow
    {
    public:
        ImguiTerraWindow( ImguiTerraContext& context ) : imguiTerraContext( context )
        {
        }

        void createWindow( Vector2i size, Vector2i pos );
        void resized( Vector2i size );
        void resizing( Vector2i size );
        void windowEvent( SDL_Event const& );
        void keyEvent( SDL_Event const&, bool pressed );
        void mouseButtonEvent( SDL_Event const&, bool pressed );
        void mouseScrollEvent( SDL_Event const& );
        void mouseMoveEvent( SDL_Event const& );

    private:
        ImguiTerraContext& imguiTerraContext;
        ImGuiContext*      imguiContext = nullptr;
    };
} // namespace terra