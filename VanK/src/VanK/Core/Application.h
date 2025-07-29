#pragma once

#include "core.h"
#include "Log.h"

namespace VanK
{
    class Window;
    class Renderer2D;
    class Event;
    class ImGuiLayer;
    class LayerStack;
    class Layer;
    class WindowResizeEvent;

    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            VK_CORE_ASSERT(index < Count, "index out of range");
            return Args[index];
        }
    };
    
    struct ApplicationSpecification
    {
        std::string Name = "VanK Application";
        std::string WorkingDirectory;
        //app args ApplicationCommandLineArgs CommandLineArgs;
        ApplicationCommandLineArgs CommandLineArgs;
    };
    
    class Application
    {
    public:
        static void SetEventCallback(const std::function<void(Event&)>& callback); // remove from here to window class ??? 
        
        Application(const ApplicationSpecification& specification);
        virtual ~Application();
    
        void OnEvent(Event& e);
        
        //Layer
        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);
        
        static Application* s_Instance;
        static LayerStack LayerStack;

        static Application& Get() { return *s_Instance; }

        const ApplicationSpecification& GetSpecification() const { return m_Specification; }

        void SubmitToMainThread(const std::function<void()>& function);
        
        void ExecuteMainThreadQueue();
        
        std::vector<std::function<void()>> m_MainThreadQueue;
        std::mutex m_MainThreadQueueMutex;
        
        ApplicationSpecification m_Specification;
    };
    
    inline bool shutdown = false;

    inline float m_LastFrameTime = 0.0f;
    
    //Events
    using EventCallbackFn = std::function<void(Event&)>;
    inline EventCallbackFn EventCallback;
    inline void Application::SetEventCallback(const EventCallbackFn& callback) { EventCallback = callback; };
    
    inline bool m_BlockEvents = false;

    inline void BlockEvents(bool block) { m_BlockEvents = block; };
    
    //Core
    inline Application* app;
    inline std::unique_ptr<Window> window = nullptr;
    //inline std::unique_ptr<Renderer> renderer = nullptr;
    inline ImGuiLayer* imguilayer = nullptr;
    
    // To be defined in CLIENT
    Application* CreateApplication(ApplicationCommandLineArgs args);
}
