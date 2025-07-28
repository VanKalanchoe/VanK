#pragma once
#include "Buffer.h"
#include "RendererAPI.h"

namespace VanK
{
    class RenderCommand
    {
    public:
        static void Init()
        {
            s_RendererAPI = RendererAPI::Create(s_Config);
            /*if (s_RendererAPI) s_RendererAPI->Init();*/
        }

        static void initImGui()
        {
            if (s_RendererAPI) s_RendererAPI->initImGui();
        }
        

        /*static void BeginFrame() { if (s_RendererAPI) s_RendererAPI->BeginFrame(); }
        static void EndFrame()   { if (s_RendererAPI) s_RendererAPI->EndFrame(); }
        static void Submit()     { if (s_RendererAPI) s_RendererAPI->Submit(); }
        static void SetLineWidth(float width) { if (s_RendererAPI) s_RendererAPI->SetLineWidth(width); }
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { if (s_RendererAPI) s_RendererAPI->SetViewport(x, y, width, height); }
        static void Clear() { if (s_RendererAPI) s_RendererAPI->Clear(); }*/

        static VanKCommandBuffer BeginCommandBuffer()
        {
            return s_RendererAPI ? s_RendererAPI->BeginCommandBuffer() : nullptr;
        }

        static void endFrame(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->endFrame(cmd);
        }

        /**
        * @brief Begins a compute pass with an optional vertex buffer
        *
        * To prevent vertex buffer usage while Computing provide it here or no guarantee to sync.
        * 
        * Inserts memory barriers to ensure safe access before compute operations.
        * 
        * @param cmd The Vulkan command buffer to record into.
        * @param buffer Optional vertex buffer to write to during the compute pass.
        *               If null, no write barrier is added.
        * 
        * @return A pointer to the created compute pass handle.
        */
        static VanKComputePass* BeginComputePass(VanKCommandBuffer cmd, VertexBuffer* buffer = nullptr)
        {
            return s_RendererAPI ? s_RendererAPI->BeginComputePass(cmd, buffer) : nullptr;
        }

        static void EndComputePass(VanKComputePass* computePass)
        {
            if (s_RendererAPI) s_RendererAPI->EndComputePass(computePass);
        }

        static void DispatchCompute(VanKComputePass* computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            if (s_RendererAPI) s_RendererAPI->DispatchCompute(computePass, groupCountX, groupCountY, groupCountZ);
        }

        static void BindStorageBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, VanKBuffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayElement = 0)
        {
            if (s_RendererAPI) s_RendererAPI->BindStorageBuffer(cmd, bindPoint, buffer, set, binding, arrayElement);
        }

        static void BindUniformBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, UniformBuffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayElement)
        {
            if (s_RendererAPI) s_RendererAPI->BindUniformBuffer(cmd, bindPoint, buffer, set, binding, arrayElement);
        }

        static void BeginRendering(VanKCommandBuffer cmd, const VanKColorTargetInfo* color_target_info, uint32_t num_color_targets, VanKDepthStencilTargetInfo depth_stencil_target_info)
        {
            if (s_RendererAPI) s_RendererAPI->BeginRendering(cmd, color_target_info, num_color_targets, depth_stencil_target_info);
        }

        static void SetViewport(VanKCommandBuffer cmd, uint32_t viewportCount, const VanKViewport viewport)
        {
            if (s_RendererAPI) s_RendererAPI->SetViewport(cmd, viewportCount, viewport);
        }

        static void SetScissor(VanKCommandBuffer cmd, uint32_t scissorCount, const VankRect scissor)
        {
            if (s_RendererAPI) s_RendererAPI->SetScissor(cmd, scissorCount, scissor);
        }

        static void SetLineWidth(VanKCommandBuffer cmd, float LineWidth)
        {
            if (s_RendererAPI) s_RendererAPI->SetLineWidth(cmd, LineWidth);
        }

        static void BindFragmentSamplers(VanKCommandBuffer cmd, uint32_t firstSlot, const TextureSamplerBinding* samplers, uint32_t num_bindings)
        {
            if (s_RendererAPI) s_RendererAPI->BindFragmentSamplers(cmd, firstSlot, samplers, num_bindings);
        }

        static void BindVertexBuffer(VanKCommandBuffer cmd, uint32_t first_slot, const VertexBuffer& vertexBuffer, uint32_t num_bindings)
        {
            if (s_RendererAPI) s_RendererAPI->BindVertexBuffer(cmd, first_slot, vertexBuffer, num_bindings);
        }

        static void BindIndexBuffer(VanKCommandBuffer cmd, const IndexBuffer& indexBuffer, VanKIndexElementSize elementSize)
        {
            if (s_RendererAPI) s_RendererAPI->BindIndexBuffer(cmd, indexBuffer, elementSize);
        }

        static void PushConstants(VanKCommandBuffer cmd, VanKShaderStageFlags shader_stage_flags, uint32_t slot_index, const void* data, uint32_t length)
        {
            if (s_RendererAPI) s_RendererAPI->PushConstants(cmd, shader_stage_flags, slot_index, data, length);
        }

        static void BindPipeline(VanKCommandBuffer cmd, VanKPipelineBindPoint pipelineBindPoint, VanKPipeLine pipeline)
        {
            if (s_RendererAPI) s_RendererAPI->BindPipeline(cmd, pipelineBindPoint, pipeline);
        }

        static void Draw(VanKCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        {
            if (s_RendererAPI) s_RendererAPI->Draw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
        }

        static void DrawIndexed(VanKCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        {
            if (s_RendererAPI) s_RendererAPI->DrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        static void EndRendering(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->EndRendering(cmd);
        }

        static void beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->beginDynamicRenderingToSwapchain(cmd);
        }

        static void endDynamicRenderingToSwapchain(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->endDynamicRenderingToSwapchain(cmd);
        }

        static void renderImGui(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->renderImGui(cmd);
        }

        static void BlitGBufferToSwapchain(VanKCommandBuffer cmd)
        {
            if (s_RendererAPI) s_RendererAPI->BlitGBufferToSwapchain(cmd);
        }

        static int32_t* downloadColorAttachmentEntityID()
        {
            return s_RendererAPI ? s_RendererAPI->downloadColorAttachmentEntityID() : nullptr;
        }

        static void destroyGraphicsPipeline()
        {
            if (s_RendererAPI) s_RendererAPI->destroyGraphicsPipeline();
        }

        static VanKPipeLine createGraphicsPipeline(VanKGraphicsPipelineSpecification pipelineSpecification)
        {
            return s_RendererAPI ? s_RendererAPI->createGraphicsPipeline(pipelineSpecification) : nullptr;
        }

        static VanKPipeLine createComputeShaderPipeline(VanKComputePipelineSpecification computePipelineSpecification)
        {
            return s_RendererAPI ? s_RendererAPI->createComputeShaderPipeline(computePipelineSpecification) : nullptr;
        }

        static void destroyComputePipeline()
        {
            if (s_RendererAPI) s_RendererAPI->destroyComputePipeline();
        }

        static void waitForGraphicsQueueIdle()
        {
            if (s_RendererAPI) s_RendererAPI->waitForGraphicsQueueIdle();
        }

        static void SetVSync(bool enable)
        {
            if (s_RendererAPI) s_RendererAPI->SetVSync(enable);
        }

        static bool GetVSync()
        {
            return s_RendererAPI ? s_RendererAPI->GetVSync() : false;
        }


        static bool IsWindowMinimized()
        {
            return s_RendererAPI ? s_RendererAPI->IsWindowMinimized() : false;
        }

        static void RebuildSwapchain()
        {
            if (s_RendererAPI) s_RendererAPI->RebuildSwapchain();
        }

        static void OnViewportSizeChange(const Extent2D& size)
        {
            if (s_RendererAPI) s_RendererAPI->OnViewportSizeChange(size);
        }

        static ImageHandle GetGBufferImage()
        {
            return s_RendererAPI ? s_RendererAPI->GetGBufferImage() : ImageHandle{};
        }

        static ImageHandle GetSwapchainImage()
        {
            return s_RendererAPI ? s_RendererAPI->GetSwapchainImage() : ImageHandle{};
        }

        static ImageHandle GetImGuiTextureID(uint32_t index = 0)
        {
            return s_RendererAPI ? s_RendererAPI->GetImGuiTextureID(index) : ImageHandle{};
        }

        static Extent2D GetViewportSize()
        {
            return s_RendererAPI ? s_RendererAPI->GetViewportSize() : Extent2D{};
        }

        static Extent2D GetWindowSize()
        {
            return s_RendererAPI ? s_RendererAPI->GetWindowSize() : Extent2D{};
        }

        static void SetWindowHandle(SDL_Window* window)
        {
            if (s_RendererAPI) s_RendererAPI->SetWindowHandle(window);
        }

        static int GetImageID()
        {
            return s_RendererAPI ? s_RendererAPI->GetImageID() : 0;
        }

        static void SetImageID(int id)
        {
            if (s_RendererAPI) s_RendererAPI->SetImageID(id);
        }

        static void SetConfig(const RendererAPI::Config& cfg)
        {
            s_Config = cfg;
        }

        static RendererAPI* GetRendererAPI()
        {
            return s_RendererAPI.get();
        }

    private:
        static RendererAPI::Config s_Config;
        static std::unique_ptr<RendererAPI> s_RendererAPI;
    };
}
