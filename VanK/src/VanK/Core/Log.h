#pragma once

#include "spdlog/spdlog.h"

namespace VanK
{
    class Log
    {
    public:
        static void Init();

        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// Core log macros
#define VK_CORE_TRACE(...) ::VanK::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VK_CORE_INFO(...)  ::VanK::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VK_CORE_WARN(...)  ::VanK::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VK_CORE_ERROR(...) ::VanK::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VK_CORE_FATAL(...) ::VanK::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define VK_TRACE(...)      ::VanK::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VK_INFO(...)       ::VanK::Log::GetClientLogger()->info(__VA_ARGS__)
#define VK_WARN(...)       ::VanK::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VK_ERROR(...)      ::VanK::Log::GetClientLogger()->error(__VA_ARGS__)
#define VK_FATAL(...)      ::VanK::Log::GetClientLogger()->fatal(__VA_ARGS__)