#pragma once

#include "VanK/Core/core.h"

namespace VanK
{
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        void OnImGuiRender();
        private:
        std::string m_CurrentDirectory;

        SDL_GPUTexture* m_DirectoryIcon;
        SDL_GPUTexture* m_FileIcon;
    };
}