#pragma once

#include <filesystem>

#include "VanK/Core/core.h"

#include "VanK/Renderer/Texture.h"

namespace VanK
{
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        
        void OnImGuiRender();
        
        private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;

        Ref<Texture2D> m_DirectoryIcon;
        Ref<Texture2D> m_FileIcon;
    };
}