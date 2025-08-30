#include "TextureImporter.h"

#include "VanK/Debug/Instrumentor.h"
#include "VanK/Project/Project.h"
#include "VanK/Renderer/Renderer2D.h"
#include "Vendor/stb_image/stb_image.h"

namespace VanK
{
    Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
    {
        VK_PROFILE_FUNCTION();

        return LoadTexture2D(Project::GetAssetDirectory() / metadata.FilePath);
    }

    Ref<Texture2D> TextureImporter::LoadTexture2D(const std::filesystem::path& path)
    {
        VK_PROFILE_FUNCTION();
        std::cout << "TextureImporter::LoadTexture2D" << std::endl;
        std::string pathStr = path.string();
        int w = 1;
        int h = 1;
        int comp = 0, req_comp = 0;
        //std::vector<uint8_t> pixelData;

        if (!pathStr.empty())
        {
            stbi_set_flip_vertically_on_load(true);
            
            Buffer data;
            {
                VK_PROFILE_SCOPE("stbi_load - TextureImporter::ImportTexture2D")
                data.Data = stbi_load(pathStr.c_str(), &w, &h, &comp, req_comp);
            }

            if (data.Data == nullptr)
            {
                VK_CORE_ASSERT(data.Data != nullptr, "TextureImporter::ImportTexture2D - Failed to load texture image");
                return nullptr;
            }
            //pixelData.assign(data, data + (w * h * req_comp));
            //stbi_image_free(data.Data);

            //todo think about this
            data.Size = w * h * comp;
            
            TextureSpecification spec;
            spec.Width = w;
            spec.Height = h;
            switch (comp)
            {
            case 3:
                spec.Format = ImageFormat::RGB8;
                break;
            case 4:
                spec.Format = ImageFormat::RGBA8;
                break;
            }
            
            Ref<Texture2D> texture = Texture2D::Create(spec, data, Renderer2D::m_sampler);
            data.Release();
            return texture;
        } else
        {
            // Create white 1x1 texture
            w = 1;
            h = 1;
            comp = 4; // RGBA
            req_comp = 4;

            TextureSpecification spec;
            spec.Width = w;
            spec.Height = h;
            switch (comp)
            {
            case 3:
                spec.Format = ImageFormat::RGB8;
                break;
            case 4:
                spec.Format = ImageFormat::RGBA8;
                break;
            }
            
            Buffer data;
            data.Data = static_cast<uint8_t*>(malloc(4));
            data.Size = 4;
        
            // Set pixel data to white (255, 255, 255, 255)
            uint8_t* pixelData = static_cast<uint8_t*>(data.Data);
            pixelData[0] = 255; // R
            pixelData[1] = 255; // G
            pixelData[2] = 255; // B
            pixelData[3] = 255; // A

            Ref<Texture2D> texture = Texture2D::Create(spec, data, Renderer2D::m_sampler);
            data.Release();
            return texture;
        }
    }
}
