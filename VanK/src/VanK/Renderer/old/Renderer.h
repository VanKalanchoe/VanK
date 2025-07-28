/*#pragma once

#include "VanK/Core/core.h"

namespace VanK
{
    class Shader;
    class Texture;
    class OrthographicCamera;
    
    class Renderer
    {
    public:
        static void Init(Window* window);
        static void Shutdown();
        static void BeginSubmit();
        static void BeginScene(const OrthographicCamera& camera);
        static void FlushBatch();
        static void EndScene();
        static void EndSubmit();
        static void ClearColor();
        static void SetColor(const glm::vec4& color);
  
        //Primitvies
        //Quad
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const glm::vec4& color);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const glm::vec4& color);
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, ::std::string
                             ImageName, float TilingFactor, const glm::vec4& color);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, ::std::string
                             ImageName, float TilingFactor, const glm::vec4& color);

        static SDL_GPUDevice* GetDevice() { return gpuDevice; }
        static SDL_GPUTexture* GetSwapChainTexture() { return SwapchainTexture; };
        static SDL_GPUCommandBuffer* GetGPUCommandBuffer() { return CommandBuffer; };
        static const SDL_GPUColorTargetInfo* GetColorTargetInfo() { return &ColorTargetInfo; };
        static SDL_FColor GetTargetInfoClearColor() { return color; };
        static SDL_FColor SetTargetInfoClearColor(SDL_FColor ColorNew) { return color = ColorNew; };

        typedef struct PositionTextureVertex
        {
            float x, y, z;
            float u, v;
        } PositionTextureVertex;

        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() { return QuadCount * 6; }
        };

        static void ResetStats();
        static Statistics GetStats();

        static std::unordered_map<std::string, SDL_GPUTexture*> TextureMap;
        
        private:
        static Window* window;
        static SDL_GPUDevice* gpuDevice;
        static SDL_GPUGraphicsPipeline* RenderPipeline;
        static SDL_GPUBuffer* VertexBuffer;
        static SDL_GPUBuffer* IndexBuffer;
        static SDL_GPUSampler* Sampler;
        static SDL_GPUCommandBuffer* CommandBuffer;
        static SDL_GPUTexture* SwapchainTexture;
        static SDL_GPURenderPass* RenderPass;
        static SDL_FColor color;
        static SDL_GPUColorTargetInfo ColorTargetInfo;
    };
}*/