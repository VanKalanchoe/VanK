#include "Texture.h"

#include "RendererAPI.h"
#include "Platform/Vulkan/VulkanTexture.h"
namespace VanK
{
    size_t VanK::Texture2D::s_ImGuiTextureCount = 0;
    
    std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path, std::shared_ptr<Sampler> sampler)
    {
        switch (RendererAPI::GetAPI())
        {
        case RenderAPIType::None: return nullptr;
        case RenderAPIType::Vulkan: return std::make_shared<VulkanTexture2D>(path, sampler);
        }

        return nullptr;
    }

    std::shared_ptr<Sampler> Sampler::Create(VanKSamplerCreateInfo& info)
    {
        switch (RendererAPI::GetAPI())
        {
        case RenderAPIType::None: return nullptr;
        case RenderAPIType::Vulkan: return std::make_shared<VulkanSampler>(info);
        }

        return nullptr;
    }
}