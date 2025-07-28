#include "VulkanTexture.h"

namespace VanK
{
    VulkanTexture2D::VulkanTexture2D(const std::string& path, std::shared_ptr<Sampler> sampler)
    : m_Path(path), m_VkSampler(static_cast<VulkanSampler*>(sampler.get())->GetVkSampler())
{
    VulkanRendererAPI& instance = VulkanRendererAPI::Get();

    if (instance.GetTextureCount() >= instance.GetMaxTexture())
    {
        instance.ResizeDescriptor(); //change this wont work i have multiple pipelines now
    }
        
    VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(),
                                                         instance.GetTransientCmdPool());

    int w = 1;
    int h = 1;
    std::vector<uint8_t> pixelData;
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    if (!path.empty())
    {
        // Search for the file
        auto others = std::string(SDL_GetBasePath()) + "Content/Images";
        const std::vector<std::string> searchPaths = {
            ".", "resources", "../resources", "../../resources", "../../../VanK-Editor/assets/textures", others
        };

        std::string filename = utils::findFile(path, searchPaths);
        ASSERT(!filename.empty(), "Could not load texture image!");

        stbi_set_flip_vertically_on_load(true);
        int comp, req_comp = 4;
        stbi_uc* data = stbi_load(filename.c_str(), &w, &h, &comp, req_comp);
        ASSERT(data != nullptr, "Failed to load texture image");

        pixelData.assign(data, data + (w * h * req_comp));
        stbi_image_free(data);
    }
    else
    {
        // Fallback: Create 1x1 white pixel
        w = h = 1;
        pixelData = { 255, 255, 255, 255 }; // RGBA white
    }

    // Prepare image info
    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { uint32_t(w), uint32_t(h), 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
    };

    const std::span dataSpan(pixelData.data(), pixelData.size());

    // Upload image
    utils::ImageResource image =
        instance.GetAllocator().createImageAndUploadData(cmd, dataSpan, imageInfo,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    DBG_VK_NAME(image.image);
    image.extent = { uint32_t(w), uint32_t(h) };

    // Create image view
    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };
    VK_CHECK(vkCreateImageView(instance.GetContext().getDevice(), &viewInfo, nullptr, &image.view));
    DBG_VK_NAME(image.view);

    m_Width = w;
    m_Height = h;

    utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(),
                                 instance.GetTransientCmdPool(),
                                 instance.GetContext().getGraphicsQueue().queue);

    m_TextureIndex = instance.AddTextureToPool(std::move(image));
    m_ImageResource = image;

    /*instance.GetAllocator().freeStagingBuffers();*/
}


    VulkanTexture2D::~VulkanTexture2D()
    {
        if (m_ImGuiHandle)
        {
            if ((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
            {
                ImGui_ImplVulkan_RemoveTexture(m_ImGuiHandle);
                m_ImGuiHandle = nullptr;
                --Texture2D::s_ImGuiTextureCount;
            }
        }
        
        // DO NOTHING - the renderer owns the texture
        // The renderer will clean up all textures when it's destroyed
        //maybe for ECS I have to ???
        try
        {
            // Check 1: Is the static instance valid?
            if (!VulkanRendererAPI::s_instance)
            {
                return; // Renderer already destroyed, nothing to do
            }

            VulkanRendererAPI& instance = VulkanRendererAPI::Get();

            // Check 2: Is the renderer properly initialized?
            if (!instance.IsInitialized())
            {
                return; // Renderer not initialized
            }

            // Check 3: Is our texture index valid?
            if (!instance.IsTextureValid(m_TextureIndex))
            {
                return; // Texture already removed or invalid index
            }

            // Check 4: Is the texture vector not empty?
            if (instance.GetTextureCount() == 0)
            {
                return; // No textures to remove
            }

            // All checks passed - safe to remove
            instance.RemoveTextureFromPool(m_TextureIndex);
        }
        catch (const std::exception& e)
        {
            // Log error but don't crash
            LOGW("Failed to remove texture from pool in destructor: %s", e.what());
        } catch (...)
        {
            // Catch any other exceptions
            LOGW("Unknown error in texture destructor");
        }
    }

    /*if ((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
    {*/
    ImTextureID VulkanTexture2D::getImTextureID()
    {
        //if this does make problems check gbuffer how they did it adding to vulkantextureimgui
        if (!m_ImGuiHandle && ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr
            && m_VkSampler)
        {
            m_ImGuiHandle = ImGui_ImplVulkan_AddTexture(
                m_VkSampler,
                m_ImageResource.view,
                m_ImageResource.layout
            );
            if (m_ImGuiHandle)
            {
                ++Texture2D::s_ImGuiTextureCount;
            }
        }
        return reinterpret_cast<ImTextureID>(m_ImGuiHandle);
    }

    // --- Conversion helpers (as previously provided) ---
    static VkFilter ToVkFilter(VanK_GPU_Filter filter)
    {
        switch (filter)
        {
        case VANK_GPU_FILTER_NEAREST: return VK_FILTER_NEAREST;
        case VANK_GPU_FILTER_LINEAR: return VK_FILTER_LINEAR;
        default: return VK_FILTER_LINEAR;
        }
    }

    static VkSamplerMipmapMode ToVkMipmapMode(VanK_GPU_SamplerMipmapMode mode)
    {
        switch (mode)
        {
        case VANK_GPU_SAMPLERMIPMAPMODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case VANK_GPU_SAMPLERMIPMAPMODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    static VkSamplerAddressMode ToVkAddressMode(VanK_GPU_SamplerAddressMode mode)
    {
        switch (mode)
        {
        case VANK_GPU_SAMPLERADDRESSMODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case VANK_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case VANK_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case VANK_GPU_SAMPLERADDRESSMODE_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    static VkCompareOp ToVkCompareOp(VanK_GPU_CompareOp op)
    {
        switch (op)
        {
        case VANK_GPU_COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER;
        case VANK_GPU_COMPARE_OP_LESS: return VK_COMPARE_OP_LESS;
        case VANK_GPU_COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL;
        case VANK_GPU_COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case VANK_GPU_COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER;
        case VANK_GPU_COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case VANK_GPU_COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case VANK_GPU_COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_ALWAYS;
        }
    }

    static VkBorderColor ToVkBorderColor(VanK_GPU_BorderColor color)
    {
        switch (color)
        {
        case VANK_GPU_BORDERCOLOR_FLOAT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case VANK_GPU_BORDERCOLOR_INT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        case VANK_GPU_BORDERCOLOR_FLOAT_OPAQUE_BLACK: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case VANK_GPU_BORDERCOLOR_INT_OPAQUE_BLACK: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        case VANK_GPU_BORDERCOLOR_FLOAT_OPAQUE_WHITE: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        case VANK_GPU_BORDERCOLOR_INT_OPAQUE_WHITE: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        default: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        }
    }

    VulkanSampler::VulkanSampler(VanKSamplerCreateInfo& info)
    {
        std::cout << "VulkanSampler::VulkanSampler" << std::endl;
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();

        VkSamplerCreateInfo vkInfo = {};
        vkInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        vkInfo.magFilter = ToVkFilter(info.magFilter);
        vkInfo.minFilter = ToVkFilter(info.minFilter);
        vkInfo.mipmapMode = ToVkMipmapMode(info.mipmapMode);
        vkInfo.addressModeU = ToVkAddressMode(info.addressModeU);
        vkInfo.addressModeV = ToVkAddressMode(info.addressModeV);
        vkInfo.addressModeW = ToVkAddressMode(info.addressModeW);
        vkInfo.mipLodBias = info.mipLodBias;
        vkInfo.anisotropyEnable = info.anisotropyEnable;
        vkInfo.maxAnisotropy = info.maxAnisotropy;
        vkInfo.compareEnable = info.compareEnable;
        vkInfo.compareOp = ToVkCompareOp(info.compareOp);
        vkInfo.minLod = info.minLod;
        vkInfo.maxLod = info.maxLod;
        vkInfo.borderColor = ToVkBorderColor(info.borderColor);
        vkInfo.unnormalizedCoordinates = info.unnormalizedCoordinates;

        m_VkSampler = instance.getSamplerPool(vkInfo);
    }

    VulkanSampler::~VulkanSampler()
    {
    }
}
