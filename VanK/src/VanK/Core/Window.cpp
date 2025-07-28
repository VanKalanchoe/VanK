#include "Log.h"
#include "window.h"
#include "VanK/Debug/Instrumentor.h"

namespace VanK
{
    Window::Window(Application& app) : m_App(app)
    {
        VK_PROFILE_FUNCTION();
        VK_CORE_INFO("new Window Class");
        initWindow();
    }

    Window::~Window()
    {
        VK_CORE_INFO("delete Window");
        SDL_DestroyWindow(window);
    }

    void Window::initWindow()
    {
        VK_PROFILE_FUNCTION();
        
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        }

        window = SDL_CreateWindow("examples/renderer/clear", 1600, 900, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        if (!window) {
            SDL_Log("Couldn't create window: %s", SDL_GetError());
        }
        
        SDL_SetWindowMinimumSize(window,640,480);
    }
}
