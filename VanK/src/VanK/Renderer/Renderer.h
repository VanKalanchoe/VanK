#pragma once

#include "EditorCamera.h"
#include "Renderer2D.h"

#include "VanK/Core/Window.h"

namespace VanK
{
    class Renderer
    {
    public:
        static void Init(Window* window);
        static void Shutdown();
        
        static void BeginSubmit();
        static void EndSubmit();
        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void FlushBatch();
        static void EndScene();
        static void RecordGraphicCommands(VanKCommandBuffer cmd);
        static SDL_AppResult DrawFrame();

    public:
        static ShaderLibrary& GetShaderLibrary() { return m_ShaderLibrary; }
        static void SetViewportSize(float width, float height) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
        }
        static glm::vec2 GetViewportSize() { return {m_ViewportWidth, m_ViewportHeight}; }
        static VanKCommandBuffer GetCommandBuffer() { return cmd; }
        inline static std::unique_ptr<UniformBuffer> m_sceneInfosBuffer;
    private:
        static void WatchShaderFiles();
        static void ReloadGraphicPipeline();
    private:
        inline static Window* window = nullptr; // Init Vulkan Surface and Imgui
        inline static float m_ViewportWidth = 800;
        inline static float m_ViewportHeight = 600;
        
        inline static VanKCommandBuffer cmd = nullptr;
        
        inline static ShaderLibrary m_ShaderLibrary;
        inline static VanKPipeLine m_GraphicsMeshPipeline = {};
        inline static VanKGraphicsPipelineSpecification m_GraphicsMeshPipelineSpecification = {};
        
        
        inline static std::unique_ptr<IndexBuffer> m_MeshIndexBuffer;
        inline static std::unique_ptr<VertexBuffer> m_CubeVertexBuffer;

        inline static shaderio::SceneInfo sceneInfos{};
    };
}