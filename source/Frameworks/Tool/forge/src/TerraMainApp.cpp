
#include <SDL.h>
#include <TerraMainApp.h>

#include <iostream>
#include <stdexcept>

namespace terra
{
    TerraMainApp::TerraMainApp()
    {
    }

    TerraMainApp::~TerraMainApp()
    {
        SDL_Quit();
    }

    void TerraMainApp::initalize()
    {
        if( SDL_Init( SDL_INIT_EVENTS ) )
        {
            throw std::runtime_error( "SDL init failed." );
        }
    }
    void TerraMainApp::updateMonitorScaling()
    {
        std::ostream* verbose  = settings.verbose ? Debug::output() : nullptr;
        int           displays = SDL_GetNumVideoDisplays();
        dpiScaling.resize( displays );
        for( int i = 0; i < displays; ++i )
        {
            Vector2 dpi;
            if( SDL_GetDisplayDPI( i, nullptr, &dpi.x(), &dpi.y() ) == 0 )
            {
                Vector2 scaling { dpi / 96.0f };
                Debug { verbose } << "Platform::Sdl2Application: virtual DPI scaling" << scaling;
                dpiScaling[i] = dpi;
            }
        }
    }

    void TerraMainApp::createContext()
    {
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 32 );

        SDL_Window* window = nullptr;
        if( !( window = SDL_CreateWindow( "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 16, 16,
                                          SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS ) ) )
        {
            throw std::runtime_error( "createContext(): cannot create window" );
        }

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

        glContext = SDL_GL_CreateContext( window );
        if( !glContext )
        {
            throw std::runtime_error( "createContext(): cannot create glContext" );
        }

        GL::Context::Configuration configuration;
        context.emplace( NoCreateT( NoCreateT::Init() ) );
        if( !context->tryCreate( configuration ) )
        {
            throw std::runtime_error( "createContext(): failed to create magnum context" );
        }
    }

    void TerraMainApp::run()
    {
        initalize();
        createContext();
        do
        {
            if( !imguiContext.pollEvents() )
                return;
            draw();
        } while( true );
    }

    int TerraMainApp::Main( int argc, const char* argv[] )
    {
        TerraMainApp app;
        try
        {
            app.run();
        }
        catch( std::exception& ex )
        {
            std::cerr << ex.what();
            std::exit( -1 );
        }
        return 0;
    }

    void TerraMainApp::draw()
    {
    }
} // namespace terra

int main( int argc, char** argv )
{
    return terra::TerraMainApp::Main( argc, (const char**)argv );
}
