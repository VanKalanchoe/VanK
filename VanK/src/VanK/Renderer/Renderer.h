#pragma once

#include "EditorCamera.h"
#include "Renderer2D.h"

#include "VanK/Core/Window.h"

namespace VanK
{
    class Renderer
    {
        #define UPLOAD_ARRAY_TO_RING_BUFFER(cmd, ringBuffer, targetBuffer, array, ElementType) \
            do { \
                uint64_t offset; \
                const size_t dataSize = sizeof(array); \
                ElementType* dataPtr = static_cast<ElementType*>(ringBuffer->MapTransferBuffer(dataSize, alignof(ElementType), offset)); \
                memcpy(dataPtr, array, dataSize); \
                ringBuffer->UnMapTransferBuffer(); \
                ringBuffer->UploadToGPUBuffer(cmd, VanKTransferBufferLocation{.offset = offset}, \
                VanKBufferRegion{.buffer = targetBuffer.get(), .offset = 0, .size = dataSize}); \
        } while(0)

        #define UploadBufferToGpuWithTransferRing(cmd, ringBuffer, targetBuffer, vector, ElementType) \
        do { \
            if (!vector.empty()) { \
                uint64_t offset; \
                const size_t dataSize = vector.size() * sizeof(ElementType); \
                ElementType* dataPtr = static_cast<ElementType*>(ringBuffer->MapTransferBuffer(dataSize, alignof(ElementType), offset)); \
                memcpy(dataPtr, vector.data(), dataSize); \
                ringBuffer->UnMapTransferBuffer(); \
                ringBuffer->UploadToGPUBuffer(cmd, VanKTransferBufferLocation{.offset = offset}, \
                VanKBufferRegion{.buffer = targetBuffer.get(), .offset = 0, .size = dataSize}); \
            } \
        } while(0)
        
    public:
        static void Init(Window* window, bool isEditor);
        static void Shutdown();
        
        static void BeginSubmit();
        static void EndSubmit();
        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void FlushBatch();
        static void EndScene();

        
        static SDL_AppResult DrawFrame();
        static void RecordGraphicCommands(VanKCommandBuffer cmd);

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
        inline static bool m_isEditor = false;
        
        inline static VanKCommandBuffer cmd = nullptr;
        
        inline static ShaderLibrary m_ShaderLibrary;
        inline static VanKPipeLine m_GraphicsMeshPipeline = {};
        inline static VanKGraphicsPipelineSpecification m_GraphicsMeshPipelineSpecification = {};
        
        inline static Scope<TransferBuffer> m_TransferRingBuffer;
        inline static Scope<IndexBuffer> m_MeshIndexBuffer;
        inline static Scope<VertexBuffer> m_CubeVertexBuffer;

        inline static shaderio::SceneInfo sceneInfos{};
    };
}