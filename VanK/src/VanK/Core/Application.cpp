#define SDL_MAIN_USE_CALLBACKS
#include "Application.h"

#include <filesystem>
#include <backends/imgui_impl_sdl3.h>
#include <SDL3/SDL_main.h>

#include "VanK/Debug/Instrumentor.h"
#include "VanK/Scripting/ScriptEngine.h"

namespace VanK
{
    // Define the static member variable
    LayerStack Application::LayerStack;

    void Application::OnEvent(Event& e)
    {
        VK_PROFILE_FUNCTION();
        
        for (auto it = LayerStack.rbegin(); it != LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        VK_PROFILE_FUNCTION();
        
        LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer* layer)
    {
        VK_PROFILE_FUNCTION();
        
        LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }

    void Application::SubmitToMainThread(const std::function<void()>& function)
    {
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
        
        m_MainThreadQueue.emplace_back(function);
    }

    void Application::ExecuteMainThreadQueue()
    {
        std::vector<std::function<void()>> copy;
        {
            std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
            copy = m_MainThreadQueue;
            m_MainThreadQueue.clear();
        }
        
        for (auto& func : copy)
            func();
    }

    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
    {
        VK_PROFILE_FUNCTION();

        VK_CORE_ASSERT(!s_Instance, "Application already exists");
        s_Instance = this;

        // Set working directory here
        if (!m_Specification.WorkingDirectory.empty())
            std::filesystem::current_path(m_Specification.WorkingDirectory);//maybe with sdl ?
        
        VK_CORE_INFO("Application::Application()");
        
        // Initialize resources, create window, etc.
        window = std::make_unique<Window>(*this); //make window create and give it tiltle from specification Window::Create(WindowProps(m_Specification.Name));
        SetEventCallback(VanK_BIND_EVENT_FN(Application::OnEvent)); // should be in window class window->seteventcallback
        
        //Renderer::Init(window.get());
        Renderer2D::Init(window.get());
        ScriptEngine::Init();
        
        imguilayer = new ImGuiLayer();
        PushOverlay(imguilayer);
    }

    Application::~Application()
    {
        VK_CORE_INFO("Application::~Application()");
        
        // Cleanup resources, close window, etc.

        {
            VK_PROFILE_SCOPE("ImGuiLayer SHUTDOWN");
            
            //mayber better oslutino in the future
            imguilayer->ShutDown();
        }

        {
            VK_PROFILE_SCOPE("Mono Shutdown");
            ScriptEngine::Shutdown();
        }
        
        {
            VK_PROFILE_SCOPE("Renderer Shutdown");
            
            Renderer2D::Shutdown();
        }
        
        {
            VK_PROFILE_SCOPE("Window Shutdown");
            
            if (window)
            {
                window.reset(); // Ensure window is destroyed
            }
        }
    }

    extern "C" {
    SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
    {
        SDL_SetAppMetadata("Engine Test", "1.0", "com.example.engine");

        Log::Init();

        VK_PROFILE_BEGIN_SESSION("Startup", "VanKProfile-Startup.json");

        ApplicationCommandLineArgs args;
        args.Count = argc;
        args.Args = argv;
        
        *appstate = CreateApplication(args);

        app = static_cast<Application*>(*appstate);

        VK_PROFILE_END_SESSION();
        VK_PROFILE_BEGIN_SESSION("Runtime", "VanKProfile-Runtime.json");
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* e)
    {
        ImGui_ImplSDL3_ProcessEvent(e);
        //maybe put this all in window class ?
        switch (e->type)
        {
        case SDL_EVENT_QUIT:
            {
                return SDL_APP_SUCCESS;
            }
        case SDL_EVENT_WINDOW_RESIZED:
            {
                int x, y;
                SDL_GetWindowSize(window->getWindow(), &x, &y);
                WindowResizeEvent events(x, y);
                EventCallback(events);
                return SDL_APP_CONTINUE;
            }
        case SDL_EVENT_MOUSE_MOTION:
            {
                MouseMovedEvent events(e->motion.x, e->motion.y);
                EventCallback(events);
                return SDL_APP_CONTINUE;
            }
        case SDL_EVENT_MOUSE_WHEEL:
            {
                if (!m_BlockEvents)
                {
                    MouseScrolledEvent events(e->wheel.x, e->wheel.y);
                    EventCallback(events);
                    return SDL_APP_CONTINUE;
                }
            }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                /*if (!m_BlockEvents) // if blocked then double clicking on emtpy space and then back to cube wont work remember that
                {*/
                    MouseButtonPressedEvent events(e->button.button);
                    EventCallback(events);
                    return SDL_APP_CONTINUE;
                /*}  */
            }
        case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                /*if (!m_BlockEvents) // if blocked then double clicking on emtpy space and then back to cube wont work remember that
                {*/
                    MouseButtonReleasedEvent events(e->button.button);
                    EventCallback(events);
                    return SDL_APP_CONTINUE;
                /*}*/
            }
        case SDL_EVENT_KEY_DOWN:
            {
                if (e->key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    return SDL_APP_SUCCESS;
                }

                if (!m_BlockEvents)
                {
                    KeyPressedEvent events(e->key.scancode);
                    EventCallback(events);
                    return SDL_APP_CONTINUE;
                }
            }
        default: return SDL_APP_CONTINUE;
        }
    }

    SDL_AppResult SDL_AppIterate(void* appstate)
    {
        VK_PROFILE_FUNCTION();
        
        if (!shutdown)
        {
            float time = SDL_GetTicks(); //not precise 
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            app->ExecuteMainThreadQueue();
            
            Uint32 windowFlags = SDL_GetWindowFlags(window->getWindow());

            if (!(windowFlags & SDL_WINDOW_MINIMIZED) && !(windowFlags & SDL_WINDOW_HIDDEN))
            {
                VK_PROFILE_SCOPE("RunLoop");
                {
                    Renderer2D::BeginSubmit();
                    
                    {
                        VK_PROFILE_SCOPE("LayerStack OnUpdate");
                    
                        for (Layer* layer : Application::LayerStack)
                            layer->OnUpdate(timestep);
                    }
                    
                    {
                        VK_PROFILE_SCOPE("LayerStack OnImGuiRender");
                        
                        imguilayer->Begin();
                        for (Layer* layer : Application::LayerStack)
                        {
                            layer->OnImGuiRender();
                        }
                        imguilayer->End();
                    }
                
                    Renderer2D::EndSubmit(); //not ideal wtf
                }
            } else
            {
                // #define SDL_HINT_MAIN_CALLBACK_RATE "60" maybe ?
                SDL_Delay(100); // Idle while minimized
            }
            //SDL_Delay(1000 / 60);
        }
        else
        {
            return SDL_APP_SUCCESS;
        }

        return SDL_APP_CONTINUE;
    }

    void SDL_AppQuit(void* appstate, SDL_AppResult result)
    {
        VK_PROFILE_END_SESSION();
        
        VK_PROFILE_BEGIN_SESSION("Shutdown", "VanKProfile-Shutdown.json");
        
        {
            VK_PROFILE_SCOPE("Application Shutdown");
            
            delete app;
        }
        
        VK_PROFILE_END_SESSION();
    }
    }
}
