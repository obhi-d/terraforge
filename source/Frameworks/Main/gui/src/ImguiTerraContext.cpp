
#include <SDL.h>
#include <ImguiTerraContext.h>
#include <ImguiTerraWindow.h>

namespace terra
{

    bool ImguiTerraContext::pollEvents()
    {
        SDL_Event event;
        bool      handled = false;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
            case SDL_WINDOWEVENT:
            {
                ImguiTerraWindow* window = nullptr;
                auto              it     = windowMap.find( event.window.windowID );
                if( it != windowMap.end() )
                    window = windows[it->second].get();

                switch( event.window.event )
                {
                /* Not using SDL_WINDOWEVENT_RESIZED, because that doesn't
                   get fired when the window is resized programmatically
                   (such as through setMaxWindowSize()) */
                case SDL_WINDOWEVENT_RESIZED:
                    if( window )
                        window->resized( Vector2i( event.window.data1, event.window.data2 ) );
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    if( window )
                        window->resizing( Vector2i( event.window.data1, event.window.data2 ) );
                    break;
                default:
                    if( window )
                        window->windowEvent( event );
                    break;
                }
            }
            break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                ImguiTerraWindow* window = nullptr;
                auto              it     = windowMap.find( event.key.windowID );
                if( it != windowMap.end() )
                {
                    window = windows[it->second].get();
                    window->keyEvent( event, event.type == SDL_KEYDOWN );
                }
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                ImguiTerraWindow* window = nullptr;
                auto              it     = windowMap.find( event.button.windowID );
                if( it != windowMap.end() )
                {
                    window = windows[it->second].get();
                    window->mouseButtonEvent( event, event.type == SDL_MOUSEBUTTONDOWN );
                }
            }
            break;
            case SDL_MOUSEWHEEL:
            {
                ImguiTerraWindow* window = nullptr;
                auto              it     = windowMap.find( event.button.windowID );
                if( it != windowMap.end() )
                {
                    window = windows[it->second].get();
                    window->mouseScrollEvent( event );
                }
            }
            break;

            case SDL_MOUSEMOTION:
            {
                ImguiTerraWindow* window = nullptr;
                auto              it     = windowMap.find( event.button.windowID );
                if( it != windowMap.end() )
                {
                    window = windows[it->second].get();
                    window->mouseMoveEvent( event );
                }
                break;
            }
            break;

            case SDL_QUIT:
            {
                return false;
            }
            break;

            /* Direct everything else to anyEvent(), so users can implement
               event handling for things not present in the Application APIs */
            default:
                break;
            }
        }

        return true;
    }
} // namespace terra