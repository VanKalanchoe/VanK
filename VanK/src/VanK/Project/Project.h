# pragma once

#include <string>
#include <filesystem>

#include "VanK/Core/core.h"

namespace VanK
{
    struct ProjectConfig
    {
        std::string Name = "Untitled";

        std::filesystem::path StartScene;

        std::filesystem::path AssetDirectory;
        std::filesystem::path ScriptModulePath;
    };
    
    class Project
    {
    public:
        static const std::filesystem::path& GetProjectDirectory()
        {
            VK_CORE_ASSERT(s_ActiveProject, "No active project");
            return s_ActiveProject->m_ProjectDirectory;
        }
        
        static std::filesystem::path GetAssetDirectory()
        {
            VK_CORE_ASSERT(s_ActiveProject, "No active project");
            return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
        }
        
        // todo move to asset manager when we have one
        static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
        {
            VK_CORE_ASSERT(s_ActiveProject, "No active project");
            return GetAssetDirectory() / path;
        }
        
        ProjectConfig& GetConfig() { return m_Config; }

        static Ref<Project> GetActive() { return s_ActiveProject; }

        static Ref<Project> New();
        static Ref<Project> Load(const std::filesystem::path& path);
        static bool SaveActive(const std::filesystem::path& path);

    private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectDirectory;

        inline static Ref<Project> s_ActiveProject;
    };
}
