/*
#pragma once

#include "Camera.h"
#include "VanK/Core/core.h"

namespace VanK
{
    struct SpriteRendererComponent;
    struct TransformComponent;
    class Shader;
    class Texture;
    class OrthographicCamera;
    class EditorCamera;
    
    class Renderer2D
    {
    public:
        static void Init(Window* window);
        static void Shutdown();
        static void CoreFrameBufferTextureResize(uint32_t width, uint32_t height);
        static void BeginSubmit();
        static void BeginScene(const ::VanK::Camera& camera, float nearPlane, float farPlane, const glm::mat4& transform);
        static void BeginScene(const EditorCamera& camera, float nearPlane, float farPlane);
        static void BeginScene(const OrthographicCamera& camera); //todo remove
        static void FlushBatch();
        static void EndScene();
        static void setDownloadedData(Uint32* id_dataRaw) { id_data = id_dataRaw; };//editor only
        static Uint32* getDownloadedData() { return id_data; };//editor only
        static glm::vec2 getSceneSize() { return { SceneWidth, SceneHeight}; };//editor only
        static void setEditorMode(bool mode) { EditorMode = mode; };//editor only
        static void EndSubmit();

        static void setViewPort(const SDL_GPUViewport& viewports) {  viewport = viewports; };
        static const SDL_GPUViewport* getViewPort() {  return &viewport; };
        
        static void ClearColor();

        static void SetColor(const glm::vec4& color);

        //Primitvies
        //Quad
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize, glm::vec2 tileMultiplier, glm::vec2 atlasSize);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize, glm::vec2 tileMultiplier, glm::vec2 atlasSize);
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, int entityID = -1);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, int entityID = -1);
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const ::std::
                             string
                             & ImageName, float TilingFactor, const glm::vec4& color, int entityID);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, ::std::string
                             ImageName, float TilingFactor, const glm::vec4& color, int entityID);

        static void DrawSprite(TransformComponent tc, SpriteRendererComponent src, int entityID);

        typedef struct PositionTextureColorVertex
        {
            float x, y, z, w;
            float u, v, padding_a, padding_b;
            float r, g, b, a;
            glm::vec3 padding_c;
            int entityID;
        } PositionTextureColorVertex;

        //effect
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
        static SDL_GPUViewport viewport;
        static void ResetStats();
        static Statistics GetStats();

        static SDL_GPUDevice* getDevice() { return device; }
        static SDL_GPUTexture* getSwapChainTexture() { return swapchainTexture; };
        static SDL_GPUCommandBuffer* getGPUCommandBuffer() { return cmdBuf; };
        static const SDL_GPUColorTargetInfo* getColorTargetInfo() { return &color_target_info; };
        static SDL_FColor getTargetInfoClearColor() { return color; };
        static SDL_FColor setTargetInfoClearColor(SDL_FColor colorNew) { return color = colorNew; };
        static std::unordered_map<std::string, SDL_GPUTexture*> TextureMap;
        static SDL_GPUTexture* SceneColorTexture;
        static SDL_GPUSampler* Sampler;
        static SDL_GPUTexture* depthStencilTexture;
        static SDL_GPUTexture* imguiTexture;static SDL_GPUTexture* swapchainTexture;
        private:
        static SDL_GPUDevice* device;
        static Window* window;
        static SDL_GPUGraphicsPipeline* RenderPipeline;
        static SDL_GPUGraphicsPipeline* EffectPipeline;
        static SDL_GPUComputePipeline* ComputePipeline;
       
        static SDL_GPUTransferBuffer* SpriteComputeTransferBuffer;
        static SDL_GPUBuffer* SpriteComputeBuffer;
        static SDL_GPUBuffer* SpriteVertexBuffer;
        static SDL_GPUBuffer* SpriteIndexBuffer;
        static SDL_GPUBuffer* EffectVertexBuffer;
        static SDL_GPUBuffer* EffectIndexBuffer;
        static SDL_GPUBuffer* TextureIndex;

        
        static SDL_GPUCommandBuffer* cmdBuf;
        static SDL_GPUColorTargetInfo color_target_info;
        static SDL_FColor color;
        
        static SDL_GPUTexture* pixelEntityID;
        static SDL_GPUTransferBuffer* downloadTransferBuffer;
        static Uint32* id_data;
        static bool EditorMode;
        
        static int SceneWidth, SceneHeight;
    };
}
*/
