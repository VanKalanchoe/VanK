#pragma once
#include <memory>
#include <string>
#include <imgui.h>
namespace VanK
{
#define VANK_GPU_LOD_CLAMP_NONE 1000.0f

    // Filtering mode
    typedef enum VanK_GPU_Filter
    {
        VANK_GPU_FILTER_NEAREST,
        VANK_GPU_FILTER_LINEAR
    } VanK_GPU_Filter;

    // Mipmap mode
    typedef enum VanK_GPU_SamplerMipmapMode
    {
        VANK_GPU_SAMPLERMIPMAPMODE_NEAREST,
        VANK_GPU_SAMPLERMIPMAPMODE_LINEAR
    } VanK_GPU_SamplerMipmapMode;

    // Addressing mode
    typedef enum VanK_GPU_SamplerAddressMode
    {
        VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
        VANK_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
        VANK_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        VANK_GPU_SAMPLERADDRESSMODE_CLAMP_TO_BORDER
    } VanK_GPU_SamplerAddressMode;

    // Compare operation
    typedef enum VanK_GPU_CompareOp
    {
        VANK_GPU_COMPARE_OP_NEVER,
        VANK_GPU_COMPARE_OP_LESS,
        VANK_GPU_COMPARE_OP_EQUAL,
        VANK_GPU_COMPARE_OP_LESS_OR_EQUAL,
        VANK_GPU_COMPARE_OP_GREATER,
        VANK_GPU_COMPARE_OP_NOT_EQUAL,
        VANK_GPU_COMPARE_OP_GREATER_OR_EQUAL,
        VANK_GPU_COMPARE_OP_ALWAYS
    } VanK_GPU_CompareOp;

    // Border color
    typedef enum VanK_GPU_BorderColor
    {
        VANK_GPU_BORDERCOLOR_FLOAT_TRANSPARENT_BLACK,
        VANK_GPU_BORDERCOLOR_INT_TRANSPARENT_BLACK,
        VANK_GPU_BORDERCOLOR_FLOAT_OPAQUE_BLACK,
        VANK_GPU_BORDERCOLOR_INT_OPAQUE_BLACK,
        VANK_GPU_BORDERCOLOR_FLOAT_OPAQUE_WHITE,
        VANK_GPU_BORDERCOLOR_INT_OPAQUE_WHITE
    } VanK_GPU_BorderColor;

    // The full struct
    typedef struct VanKSamplerCreateInfo
    {
        VanK_GPU_Filter magFilter;
        VanK_GPU_Filter minFilter;
        VanK_GPU_SamplerMipmapMode mipmapMode;
        VanK_GPU_SamplerAddressMode addressModeU;
        VanK_GPU_SamplerAddressMode addressModeV;
        VanK_GPU_SamplerAddressMode addressModeW;
        float mipLodBias;
        int anisotropyEnable; // 0 or 1
        float maxAnisotropy;
        int compareEnable; // 0 or 1
        VanK_GPU_CompareOp compareOp;
        float minLod;
        float maxLod;
        VanK_GPU_BorderColor borderColor;
        int unnormalizedCoordinates; // 0 or 1
    } VanKSamplerCreateInfo;

    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        static std::shared_ptr<Sampler> Create(VanKSamplerCreateInfo& info);
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetTextureIndex() const = 0;

        virtual const std::string& GetPath() const = 0;
    };

    class Texture2D : public Texture
    {
    public:
        virtual ImTextureID getImTextureID() = 0;
        static size_t GetNumImGuiTextures() { return s_ImGuiTextureCount; }
        static std::shared_ptr<Texture2D> Create(const std::string& path, std::shared_ptr<Sampler> sampler);
        static size_t s_ImGuiTextureCount;
    };
}
