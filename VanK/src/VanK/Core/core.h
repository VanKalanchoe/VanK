#pragma once

//---------3rd Vendor-----
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  // for pointer to matrix or vector
//------------------------

//----------Misc----------
#include <iostream>
#include <sstream>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <optional>
#include <memory>
//------------------------

//---------Event def------
#define BIT(x) (1 << (x))
#define VanK_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
//------------------------

//---------Assert def-----
#define VK_ENABLE_ASSERTS // in cmake not here
#ifdef VK_ENABLE_ASSERTS
    #define VK_ASSERT(x, ...) { if(!(x)) { VK_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak; } } //__debugbreak() is windows only __builtin_trap() for linux macos
    #define VK_CORE_ASSERT(x, ...) { if(!(x)) { VK_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak; } }
#else
    #define VK_ASSERT(x, ...)
    #define VK_CORE_ASSERT(x, ...)
#endif

//------------------------

//---------Pointer--------
namespace VanK
{
    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}
//------------------------

#include "Application.h"

#include "Window.h"

//#include "VanK/Renderer/Renderer.h"
#include "VanK/Renderer/Renderer2D.h"

#include "VanK/Renderer/Shader.h"

#include "VanK/Renderer/Texture.h"

#include "VanK/Events/Event.h"

#include "VanK/Events/KeyEvent.h"

#include "VanK/Events/ApplicationEvent.h"

#include "VanK/Events/MouseEvent.h"

#include "Layer.h"

#include "VanK/ImGui/ImGuiLayer.h"

#include "LayerStack.h"

#include "VanK/Renderer/OrthographicCamera.h"

#include "VanK/core/Timestep.h"

#include "VanK/Scene/Scene.h"