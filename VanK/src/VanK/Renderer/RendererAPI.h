#pragma once

#include <memory>
#include <optional>

#include "SDL3/SDL.h"
#include "SDL3/SDL_stdinc.h"
#include "Buffer.h"
#include "Texture.h"
#include "Platform/Vulkan/VulkanShader.h"//remove use shader and downcast in pipeline or move pipeline into renderer

namespace VanK
{
    class VanKBuffer;
    class VertexBuffer;
    class IndexBuffer;
    class UniformBuffer;

    // Forward declare an opaque struct (incomplete type)
    struct VanKCommandBuffer_T;
    using VanKCommandBuffer = VanKCommandBuffer_T*;

    struct VanKPipeLine_T;
    using VanKPipeLine = VanKPipeLine_T*;

    enum VanKShaderStageFlags
    {
        VanKGraphics,
        VanKCompute
    };

    struct VanKSpecializationMapEntries
    {
        uint32_t constantID;
        uint32_t offset;
        size_t size;
    };

    struct VanKSpecializationInfo
    {
        std::vector<VanKSpecializationMapEntries> MapEntries;     // Owns the entries
        std::vector<uint8_t> Data;                                // Owns the raw data

        size_t dataSize() const { return Data.size(); }
        uint32_t mapEntryCount() const { return static_cast<uint32_t>(MapEntries.size()); }

        const void* getData() const { return Data.data(); }
        const VanKSpecializationMapEntries* getEntries() const { return MapEntries.data(); }
    };
    
    struct VanKPipelineShaderStageCreateInfo
    {
        Shader* VanKShader;
        std::optional<VanKSpecializationInfo> specializationInfo;
    };

    struct VanKPipelineVertexInputStateCreateInfo
    {
        VertexBuffer* VanKBuffer;
    };

    enum VankPrimitiveToplogy
    {
        VanK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VanK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VanK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        VanK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        VanK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
        VanK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
        VanK_PRIMITIVE_TOPOLOGY_MAX_ENUM
    };

    struct VanKPipelineInputAssemblyStateCreateInfo
    {
        VankPrimitiveToplogy VanKPrimitive;
    };

    enum VanKPolygonMode
    {
        VanK_POLYGON_MODE_FILL,
        VanK_POLYGON_MODE_LINE,
        VanK_POLYGON_MODE_POINT,
        VanK_POLYGON_MODE_FILL_RECTANGLE_NV,
        VanK_POLYGON_MODE_MAX_ENUM
    };

    enum VanKCullModeFlags
    {
        VanK_CULL_MODE_NONE,
        VanK_CULL_MODE_FRONT_BIT,
        VanK_CULL_MODE_BACK_BIT,
        VanK_CULL_MODE_FRONT_AND_BACK,
        VanK_CULL_MODE_FLAG_BITS_MAX_ENUM
    };

    enum VanKFrontFace
    {
        VanK_FRONT_FACE_COUNTER_CLOCKWISE,
        VanK_FRONT_FACE_CLOCKWISE,
        VanK_FRONT_FACE_MAX_ENUM
    };
    
    struct VanKPipelineRasterizationStateCreateInfo
    {
        VanKPolygonMode VanKPolygon;
        VanKCullModeFlags VanKCullMode;
        VanKFrontFace VanKFrontFace;
    };
    
    struct VanKGraphicsPipelineSpecification
    {
        VanKPipelineShaderStageCreateInfo ShaderStageCreateInfo;
        VanKPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
        VanKPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo;
        VanKPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;
    };

    struct VanKComputePipelineCreateInfo
    {
        Shader* VanKShader;
    };

    struct VanKComputePipelineSpecification
    {
        VanKComputePipelineCreateInfo ComputePipelineCreateInfo;
    };

    struct VanKComputePass
    {
        VanKCommandBuffer VanKCommandBuffer;
        VertexBuffer* VanKVertexBuffer;
    };

    enum class VanKPipelineBindPoint
    {
        Graphics,
        Compute
    };
    
    enum class VanKIndexElementSize
    {
        Uint16,
        Uint32
    };

    struct TextureSamplerBinding {
        const Texture2D* texture;
        const Sampler* sampler;
    };

    enum VanKTextureFormat {
        VanK_TEXTUREFORMAT_INVALID,
        VanK_TEXTUREFORMAT_A8_UNORM,
        VanK_TEXTUREFORMAT_R8_UNORM,
        VanK_TEXTUREFORMAT_R8G8_UNORM,
        VanK_TEXTUREFORMAT_R8G8B8A8_UNORM,
        VanK_TEXTUREFORMAT_R16_UNORM,
        VanK_TEXTUREFORMAT_R16G16_UNORM,
        VanK_TEXTUREFORMAT_R16G16B16A16_UNORM,
        VanK_TEXTUREFORMAT_R10G10B10A2_UNORM,
        VanK_TEXTUREFORMAT_B5G6R5_UNORM,
        VanK_TEXTUREFORMAT_B5G5R5A1_UNORM,
        VanK_TEXTUREFORMAT_B4G4R4A4_UNORM,
        VanK_TEXTUREFORMAT_B8G8R8A8_UNORM,
        VanK_TEXTUREFORMAT_BC1_RGBA_UNORM,
        VanK_TEXTUREFORMAT_BC2_RGBA_UNORM,
        VanK_TEXTUREFORMAT_BC3_RGBA_UNORM,
        VanK_TEXTUREFORMAT_BC4_R_UNORM,
        VanK_TEXTUREFORMAT_BC5_RG_UNORM,
        VanK_TEXTUREFORMAT_BC7_RGBA_UNORM,
        VanK_TEXTUREFORMAT_BC6H_RGB_FLOAT,
        VanK_TEXTUREFORMAT_BC6H_RGB_UFLOAT,
        VanK_TEXTUREFORMAT_R8_SNORM,
        VanK_TEXTUREFORMAT_R8G8_SNORM,
        VanK_TEXTUREFORMAT_R8G8B8A8_SNORM,
        VanK_TEXTUREFORMAT_R16_SNORM,
        VanK_TEXTUREFORMAT_R16G16_SNORM,
        VanK_TEXTUREFORMAT_R16G16B16A16_SNORM,
        VanK_TEXTUREFORMAT_R16_FLOAT,
        VanK_TEXTUREFORMAT_R16G16_FLOAT,
        VanK_TEXTUREFORMAT_R16G16B16A16_FLOAT,
        VanK_TEXTUREFORMAT_R32_FLOAT,
        VanK_TEXTUREFORMAT_R32G32_FLOAT,
        VanK_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        VanK_TEXTUREFORMAT_R11G11B10_UFLOAT,
        VanK_TEXTUREFORMAT_R8_UINT,
        VanK_TEXTUREFORMAT_R8G8_UINT,
        VanK_TEXTUREFORMAT_R8G8B8A8_UINT,
        VanK_TEXTUREFORMAT_R16_UINT,
        VanK_TEXTUREFORMAT_R16G16_UINT,
        VanK_TEXTUREFORMAT_R16G16B16A16_UINT,
        VanK_TEXTUREFORMAT_R32_UINT,
        VanK_TEXTUREFORMAT_R32G32_UINT,
        VanK_TEXTUREFORMAT_R32G32B32A32_UINT,
        VanK_TEXTUREFORMAT_R8_INT,
        VanK_TEXTUREFORMAT_R8G8_INT,
        VanK_TEXTUREFORMAT_R8G8B8A8_INT,
        VanK_TEXTUREFORMAT_R16_INT,
        VanK_TEXTUREFORMAT_R16G16_INT,
        VanK_TEXTUREFORMAT_R16G16B16A16_INT,
        VanK_TEXTUREFORMAT_R32_INT,
        VanK_TEXTUREFORMAT_R32G32_INT,
        VanK_TEXTUREFORMAT_R32G32B32A32_INT,
        VanK_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        VanK_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB,
        VanK_TEXTUREFORMAT_BC1_RGBA_UNORM_SRGB,
        VanK_TEXTUREFORMAT_BC2_RGBA_UNORM_SRGB,
        VanK_TEXTUREFORMAT_BC3_RGBA_UNORM_SRGB,
        VanK_TEXTUREFORMAT_BC7_RGBA_UNORM_SRGB,
        VanK_TEXTUREFORMAT_D16_UNORM,
        VanK_TEXTUREFORMAT_D24_UNORM,
        VanK_TEXTUREFORMAT_D32_FLOAT,
        VanK_TEXTUREFORMAT_D24_UNORM_S8_UINT,
        VanK_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
        VanK_TEXTUREFORMAT_ASTC_4x4_UNORM,
        VanK_TEXTUREFORMAT_ASTC_5x4_UNORM,
        VanK_TEXTUREFORMAT_ASTC_5x5_UNORM,
        VanK_TEXTUREFORMAT_ASTC_6x5_UNORM,
        VanK_TEXTUREFORMAT_ASTC_6x6_UNORM,
        VanK_TEXTUREFORMAT_ASTC_8x5_UNORM,
        VanK_TEXTUREFORMAT_ASTC_8x6_UNORM,
        VanK_TEXTUREFORMAT_ASTC_8x8_UNORM,
        VanK_TEXTUREFORMAT_ASTC_10x5_UNORM,
        VanK_TEXTUREFORMAT_ASTC_10x6_UNORM,
        VanK_TEXTUREFORMAT_ASTC_10x8_UNORM,
        VanK_TEXTUREFORMAT_ASTC_10x10_UNORM,
        VanK_TEXTUREFORMAT_ASTC_12x10_UNORM,
        VanK_TEXTUREFORMAT_ASTC_12x12_UNORM,
        VanK_TEXTUREFORMAT_ASTC_4x4_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_5x4_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_5x5_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_6x5_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_6x6_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_8x5_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_8x6_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_8x8_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_10x5_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_10x6_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_10x8_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_10x10_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_12x10_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_12x12_UNORM_SRGB,
        VanK_TEXTUREFORMAT_ASTC_4x4_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_5x4_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_5x5_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_6x5_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_6x6_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_8x5_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_8x6_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_8x8_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_10x5_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_10x6_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_10x8_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_10x10_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_12x10_FLOAT,
        VanK_TEXTUREFORMAT_ASTC_12x12_FLOAT
    };

    enum VanKLoadOp {
        VanK_LOADOP_LOAD = 0,
        VanK_LOADOP_CLEAR = 1,
        VanK_LOADOP_DONT_CARE = 2,
    };

    enum  VanKStoreOp
    {
        VanK_STOREOP_STORE,
        VanK_STOREOP_DONT_CARE,
        VanK_STOREOP_RESOLVE,
        VanK_STOREOP_RESOLVE_AND_STORE
    };

    struct VanK_FColor
    {
        union {
            float f[4];
            int32_t i[4];
            uint32_t u[4];
        };
    };

    struct VanKColorTargetInfo
    {
        VanKTextureFormat format;
        VanKLoadOp loadOp;
        VanKStoreOp storeOp;
        VanK_FColor clearColor;
    };

    struct VanKDepthStencilTargetInfo
    {
        VanKLoadOp loadOp;
        VanKStoreOp storeOp;
        VanK_FColor clearColor;
    };

    // In your engine's core headers
    // Abstracted engine-side types
    struct ImageHandle { void* handle = nullptr; }; // Opaque
    struct Extent2D { uint32_t width, height; };
    struct VanKViewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };
    struct VankRect
    {
        int32_t x = 0;
        int32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };


    enum class RenderAPIType
    {
        None = 0, Vulkan = 1, Metal = 2
    };

    class RendererAPI {
    public:
        virtual ~RendererAPI() = default;
        virtual void initImGui() = 0;

        /*virtual void Init() = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Submit() = 0;
        virtual void SetLineWidth(float width) = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void Clear() = 0;*/

        virtual VanKCommandBuffer BeginCommandBuffer() { return nullptr; }
        virtual void endFrame(VanKCommandBuffer cmd) = 0;

        virtual VanKComputePass* BeginComputePass(VanKCommandBuffer cmd, VertexBuffer* buffer = nullptr) = 0;

        virtual void EndComputePass(VanKComputePass* computePass) = 0;
        
        virtual void DispatchCompute(VanKComputePass* computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        // Add this declaration in the RendererAPI class (around line 236)
        virtual void BindStorageBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, VanKBuffer* buffer, uint32_t set, uint32_t binding, uint32_t
                                       arrayElement = 0) = 0;
        virtual void BindUniformBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, UniformBuffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) = 0;
        virtual void BeginRendering(VanKCommandBuffer cmd, const VanKColorTargetInfo* color_target_info, uint32_t num_color_targets,
                            VanKDepthStencilTargetInfo depth_stencil_target_info) = 0;
        virtual void SetViewport(VanKCommandBuffer cmd, uint32_t viewportCount, const VanKViewport viewport) = 0;
        virtual void SetScissor(VanKCommandBuffer cmd, uint32_t scissorCount, VankRect scissor) = 0;
        virtual void SetLineWidth(VanKCommandBuffer cmd, float LineWidth) = 0;
        virtual void BindFragmentSamplers(VanKCommandBuffer cmd, uint32_t firstSlot, const TextureSamplerBinding* samplers, uint32_t num_bindings) = 0;
        virtual void BindVertexBuffer(VanKCommandBuffer cmd, uint32_t first_slot, const VertexBuffer& vertexBuffer, uint32_t num_bindings) = 0;
        virtual void BindIndexBuffer(VanKCommandBuffer cmd, const IndexBuffer& indexBuffer, VanKIndexElementSize elementSize) = 0;
        virtual void PushConstants(VanKCommandBuffer cmd, VanKShaderStageFlags stageFlags, uint32_t slot_index, const void* data, uint32_t length) = 0;
        virtual void BindPipeline(VanKCommandBuffer cmd, VanKPipelineBindPoint pipelineBindPoint, VanKPipeLine pipeline) = 0;
        virtual void Draw(VanKCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(VanKCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void EndRendering(VanKCommandBuffer cmd) = 0;
    
        virtual void beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd) = 0;
        virtual void endDynamicRenderingToSwapchain(VanKCommandBuffer cmd) = 0;
        virtual void renderImGui(VanKCommandBuffer cmd) = 0;
        virtual void BlitGBufferToSwapchain(VanKCommandBuffer cmd) = 0;
        virtual int32_t* downloadColorAttachmentEntityID() = 0;
        virtual void destroyGraphicsPipeline() = 0;
        virtual VanKPipeLine createGraphicsPipeline(VanKGraphicsPipelineSpecification pipelineSpecification) = 0;
        virtual VanKPipeLine createComputeShaderPipeline(VanKComputePipelineSpecification computePipelineSpecification) = 0;
        virtual void destroyComputePipeline() = 0;
        virtual void waitForGraphicsQueueIdle() = 0;

        // In RendererAPI.h
        virtual void SetVSync(bool enable) = 0;
        virtual bool GetVSync() const = 0;
        virtual bool IsWindowMinimized() const { return false; }
        virtual void RebuildSwapchain() {}
        virtual void OnViewportSizeChange(const Extent2D& size) = 0;
    
        virtual ImageHandle GetGBufferImage() const = 0;
        virtual ImageHandle GetSwapchainImage() const = 0;
        virtual ImageHandle GetImGuiTextureID(uint32_t index = 0) const = 0;
    
        virtual Extent2D GetViewportSize() const = 0;
        virtual Extent2D GetWindowSize() const = 0;
    
        virtual void SetWindowHandle(SDL_Window* window) = 0;

        virtual int GetImageID() const = 0;
        virtual void SetImageID(int id) = 0;

        static RenderAPIType GetAPI() { return s_API; }
        static void SetAPI(RenderAPIType api) { s_API = api; }

        // --- New: Configuration ---
        struct Config {
            SDL_Window* window = nullptr;
            uint32_t width = 800;
            uint32_t height = 600;
        };
    
        static std::unique_ptr<RendererAPI> Create(const Config& config);

    private:
        static RenderAPIType s_API;
    };
}