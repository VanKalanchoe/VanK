#include "ContentBrowserPanel.h"

#include <imgui.h>

#include <SDL3/SDL_filesystem.h>

namespace VanK
{
    std::string BaseDir = SDL_GetBasePath();

    struct FileEntry
    {
        std::string directory;
        std::string filename;
    };

    // Global or member variable to store file entries
    std::vector<FileEntry> g_DirectoryEntries;
    
    /*static SDL_GPUTextureSamplerBinding m_DirectoryIconBind;
    static SDL_GPUTextureSamplerBinding m_FileIconBind;*/
    std::shared_ptr<Texture2D> m_DirectoryIconBind = nullptr;
    std::shared_ptr<Texture2D> m_FileIconBind = nullptr;
    
    static SDL_EnumerationResult SDLCALL callback(void* userdata, const char* dirname, const char* fname)
    {
        auto entries = static_cast<std::vector<FileEntry>*>(userdata);

        // Store the directory and filename separately
        FileEntry entry;
        entry.directory = dirname;
        entry.filename = fname;

        entries->push_back(entry);

        // Return SDL_ENUM_CONTINUE to keep enumerating the next entries.
        return SDL_ENUM_CONTINUE;
    }

    ContentBrowserPanel::ContentBrowserPanel() : m_CurrentDirectory(SDL_GetBasePath())
    {
        m_DirectoryIconBind = Texture2D::Create("DirectoryIcon.png", Renderer2D::m_sampler);
        m_FileIconBind = Texture2D::Create("FileIcon.png", Renderer2D::m_sampler);
        /*Texture::CreateTexture2D("DirectoryIcon.png", 4);
        Texture::CreateTexture2D("FileIcon.png", 4);
        
        m_DirectoryIcon = Renderer2D::TextureMap["DirectoryIcon.png"];
        m_FileIcon = Renderer2D::TextureMap["FileIcon.png"];
        
        m_DirectoryIconBind.texture = m_DirectoryIcon;
        m_DirectoryIconBind.sampler = Renderer2D::Sampler;
        
        m_FileIconBind.texture = m_FileIcon;
        m_FileIconBind.sampler = Renderer2D::Sampler;*/
    }

    // Function to remove the last component (filename or folder) from the path
    std::string RemoveLastComponent(std::string path)
    {
        // Remove trailing slash if it exists
        if (path.back() == '/' || path.back() == '\\')
        {
            path.pop_back(); // Remove the trailing slash
        }

        // Find the last occurrence of '/' or '\\'
        size_t pos = path.find_last_of("/\\");

        if (pos != std::string::npos)
        {
            // Return the path without the last component (file or folder)
            return path.substr(0, pos);
        }

        // If no separator is found, return the original path (root directory)
        return path;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        // Clear previous entries if needed
        g_DirectoryEntries.clear();

        if (!SDL_EnumerateDirectory(m_CurrentDirectory.c_str(), callback, &g_DirectoryEntries))
        {
            // SDL_EnumerateDirectory returns false on error (or if the callback returns SDL_ENUM_FAILURE).
            fprintf(stderr, "Error enumerating directory: %s\n", SDL_GetError());
        }

        ImGui::Begin("Content Browser");
        // Button to go back to the parent directory
        if (m_CurrentDirectory != BaseDir)
        {
            if (BaseDir.back() == '/' || BaseDir.back() == '\\')
            {
                BaseDir.pop_back(); // Ensure BaseDir has no trailing slash
            }

            if (ImGui::Button("<-"))
            {
                // Navigate to the parent directory
                m_CurrentDirectory = RemoveLastComponent(m_CurrentDirectory);
            }
        }

        static float padding = 16.0f;
        static float thumbnailSize = 128.0f;
        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1)
            columnCount = 1;

        ImGui::Columns(columnCount, 0, false);
        
        // reduce time maybe once a second in the future
        for (const auto& entry : g_DirectoryEntries)
        {
            SDL_PathInfo pathInfo = {};
            // Construct the full path for this entry
            std::string fullEntryPath = entry.directory + entry.filename;
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::ImageButton(entry.filename.c_str(), ImTextureID((SDL_GetPathInfo(fullEntryPath.c_str(), &pathInfo) == 1 && pathInfo.type == SDL_PATHTYPE_DIRECTORY) ? m_DirectoryIconBind->getImTextureID() : m_FileIconBind->getImTextureID()), ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", fullEntryPath.c_str(), fullEntryPath.size() + 1);
                ImGui::EndDragDropSource();
            }

            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (SDL_GetPathInfo(fullEntryPath.c_str(), &pathInfo) == 1 && pathInfo.type == SDL_PATHTYPE_DIRECTORY)
                {
                    m_CurrentDirectory = fullEntryPath;
                } 
            }
            ImGui::TextWrapped(entry.filename.c_str());

            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
        ImGui::SliderFloat("Padding", &padding, 0, 32);
        ImGui::End();
    }
}
