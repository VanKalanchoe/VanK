#pragma once

#include "VanK/Core/core.h"

namespace VanK
{
    class Event;
   
    class ImGuiLayer : public Layer {
    public:
        ImGuiLayer();
        virtual ~ImGuiLayer();

        virtual void OnAttach();
        virtual void OnDetach();
        void ShutDown();
        void Begin();
        virtual void OnImGuiRender();
        void End();
        void SetDarkThemeColors();

        uint32_t GetActiveWidgetID() const;
    };
}
