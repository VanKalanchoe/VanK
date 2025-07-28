#pragma once

#include <string>

#include "Timestep.h"

namespace VanK {
    class Event;
    
    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnEvent(Event& event) {}
        virtual void OnImGuiRender() {}

        const std::string& GetName() const { return m_Name; }

    protected:
        std::string m_Name;
    };

}
