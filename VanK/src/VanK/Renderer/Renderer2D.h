#pragma once

#include <SDL3/SDL_init.h>
#include "RenderCommand.h"
#include <array>
#include <glm/glm.hpp>

#include "Camera.h"
#include "VanK/Core/core.h"
#include "VanK/Scene/Components.h"

namespace VanK
{
    struct SpriteRendererComponent;
    struct CircleRendererComponent;
    struct TransformComponent;
    class OrthographicCamera;
    class EditorCamera;
    
    namespace shaderio
    {
        using namespace glm;
        #include "shader_io.h"
    }

    class Renderer2D
    {
    public:
        inline static float m_ViewportWidth = 800;
        inline static float m_ViewportHeight = 600;
        static void Init(Window* window);
        static void Shutdown();
        static void BeginSubmit();
        static void BeginScene(const ::VanK::Camera& camera, const glm::mat4& transform);
        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const OrthographicCamera& camera); //todo remove
        static void FlushBatch();
        static void EndScene();
        static void EndSubmit();
        
        static SDL_AppResult drawFrame();

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize, glm::vec2 tileMultiplier, glm::vec2 atlasSize);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize, glm::vec2 tileMultiplier, glm::vec2 atlasSize);
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, int entityID = -1);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, int entityID = -1);

        static void DrawCircle(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
        static void DrawCircle(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

        static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int EntityID = -1);
        static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int EntityID = -1);
        static void DrawRect(const glm::mat4& transform, const glm::vec4& color, int EntityID = -1);
        static void DrawRect(TransformComponent tc, SpriteRendererComponent src, int entityID = -1);
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const Ref<Texture2D>& texture, float TilingFactor, const glm::vec4& color, int entityID);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale, const glm::vec3& rotation, const Ref<Texture2D>& texture, float TilingFactor, const glm::vec4& color, int entityID);

        static void DrawSprite(TransformComponent tc, SpriteRendererComponent src, int entityID);
        static void DrawCircle(TransformComponent tc, CircleRendererComponent src, int entityID);

        inline static bool m_forceViewportResize = false;
        static void initRenderer();
        static void shutdownRenderer();
        static void watchShaderFiles();
        static void useImGui();
        static void rendererEvent(SDL_Event* event);
        static void OnViewportSizeChange(const Extent2D& newSize); // Called from SDL or main loop
        static void SetViewportSize(float width, float height) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
        }

        static void reloadGraphicsPipeline();
        static void recordComputeCommands(VanKCommandBuffer cmd);
        static void recordGraphicCommands(VanKCommandBuffer cmd);
        static void recordComputeCommandsCircles(VanKCommandBuffer cmd);
        static void recordGraphicCommandsCircles(VanKCommandBuffer cmd);
        static void recordGraphicCommandsLine(VanKCommandBuffer cmd);

        static glm::vec2 getSceneSize() { return { SceneWidth, SceneHeight}; };
        inline static int SceneWidth, SceneHeight;
        inline static bool m_useImGui = false; // Flag to control ImGui rendering
        inline static bool m_windowResized = false;
        inline static Extent2D lastViewportSize = {0, 0};
        inline static std::atomic<bool> s_NeedsPipelineReload = false;

        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() { return QuadCount * 6; }
        };
        static void ResetStats();
        static Statistics GetStats();
        
        inline static std::shared_ptr<Sampler> m_sampler = nullptr;
        
        inline static Window* window = nullptr;
        inline static VanKCommandBuffer cmd = nullptr;

        // graphics pipelines
        inline static VanKPipeLine textureGraphicsPipeline = {};
        inline static VanKGraphicsPipelineSpecification m_graphicsTexturePipelineSpecification = {};
        inline static VanKPipeLine circleGraphicsPipeline = {};
        inline static VanKGraphicsPipelineSpecification m_graphicsCirclePipelineSpecification = {};
        inline static VanKPipeLine lineGraphicsPipeline = {};
        inline static VanKGraphicsPipelineSpecification m_lineGraphicsPipelineSpecification = {};
        
        // compute pipelines
        inline static VanKPipeLine textureComputePipeline = {};
        inline static VanKComputePipelineSpecification m_computeTexturePipelineSpecification = {};
        inline static VanKPipeLine circleComputePipeline = {};
        inline static VanKComputePipelineSpecification m_computeCirclePipelineSpecification = {};
    };
}