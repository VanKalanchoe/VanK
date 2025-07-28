#pragma once
#include "core.h"

namespace VanK
{
    //forward declare
    class Application;
    
    class Window
    {
    public:
        Window(Application& app);
        ~Window();
        
        void initWindow();
        virtual SDL_Window* getWindow() { return window; };
    private:
        SDL_Window* window = nullptr;
        Application& m_App; // Store the reference to Application
    };
}
