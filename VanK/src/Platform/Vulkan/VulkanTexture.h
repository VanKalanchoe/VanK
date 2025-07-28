#pragma once

#include "VanK/Renderer/Texture.h"
#include "VulkanRendererAPI.h"
namespace VanK
{
    class VulkanTexture2D : public Texture2D
    {
    public:
        /*VulkanTexture2D(const std::string& path);*/
        VulkanTexture2D(const std::string& path, std::shared_ptr<Sampler> sampler);
        virtual ~VulkanTexture2D() override;

        virtual uint32_t GetWidth() const override { return m_Width; };
        virtual uint32_t GetHeight() const override { return m_Height; };
        virtual uint32_t GetTextureIndex() const override { return m_TextureIndex; }
        virtual ImTextureID getImTextureID() override;
        virtual const std::string& GetPath() const override { return m_Path; };
    private:
        std::string m_Path;//maybe deleted when asset manaager exists only here for hot reloading in the future 
        uint32_t m_Width, m_Height;
        uint32_t m_TextureIndex = 0;
        VkDescriptorSet m_ImGuiHandle = nullptr;
        VkSampler m_VkSampler;
        utils::ImageResource m_ImageResource;
    };

    class VulkanSampler : public Sampler
    {
    public:
        VulkanSampler(VanKSamplerCreateInfo& info);
        virtual ~VulkanSampler();

        VkSampler GetVkSampler() const { return m_VkSampler; }

    private:
        VkSampler m_VkSampler;
    };
}