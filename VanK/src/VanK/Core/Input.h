#pragma once

#include "core.h"

namespace VanK
{
    class Input
    {
    public:  //replace with custom keycode
        static bool IsKeyPressed(SDL_Keycode key);

        static bool IsMouseButtonPressed(SDL_MouseButtonFlags button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
        static float x;
        static float y;
    };
}