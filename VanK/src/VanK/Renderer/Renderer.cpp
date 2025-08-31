#include "Renderer.h"

#include "VanK/Core/Application.h"
#include "VanK/Core/Timer.h"
#include "VanK/Debug/Instrumentor.h"
#include "VanK/Renderer/Renderer2D.h"

#include <Vendor/FileWatch/FileWatch.h>

#include "VanK/Asset/TextureImporter.h"

namespace VanK
{
    static std::vector<std::unique_ptr<filewatch::FileWatch<std::string>>> s_ShaderWatcher;
    static std::atomic<bool> s_IsPipelineReloadFinished = false;
    bool IsShaderReloadFinished = false;
    std::string changedFile;
    Timer ReloadTimer;
    
    void Renderer::WatchShaderFiles()
    {
        for (const std::string& path : GetShaderLibrary().GetAllShaderPaths())
        {
            s_ShaderWatcher.emplace_back(std::make_unique<filewatch::FileWatch<std::string>>(path,
                [](const std::string& file, const filewatch::Event change_type)
                {
                    if (!IsShaderReloadFinished && change_type == filewatch::Event::modified)
                    {
                        std::cout << "[FileWatcher] Shader file changed: " << file << '\n';

                        IsShaderReloadFinished = true;

                        changedFile = file;

                        ReloadTimer = Timer();
                    
                        Application::Get().SubmitToMainThread([]()
                        {
                            s_ShaderWatcher.clear();
                            s_IsPipelineReloadFinished = true;
                        });
                    }
                }));
        }
    }

    void Renderer::ReloadGraphicPipeline()
    {
        VK_CORE_WARN("Reloading took {}ms", ReloadTimer.ElapsedMillis());
        
        if (changedFile == "GraphicsMeshShader.slang")
        {
            RenderCommand::waitForGraphicsQueueIdle();
            RenderCommand::DestroyPipeline(m_GraphicsMeshPipeline);
            GetShaderLibrary().Remove("MeshShader");
            auto MeshShader = GetShaderLibrary().Load("MeshShader", changedFile);

            //graphics
            m_GraphicsMeshPipelineSpecification.ShaderStageCreateInfo.VanKShader = MeshShader;
        
            m_GraphicsMeshPipeline = RenderCommand::createGraphicsPipeline(m_GraphicsMeshPipelineSpecification);
        }

        Renderer2D::reloadGraphicsPipeline(changedFile);
    }
    
    struct QuadVertex
    {
        glm::vec3 Position;
        int pad0;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
		
        // Editor-only
        int EntityID;
    };

    struct Renderer3DData
    {
        static const uint32_t MaxCubes = 500;
        static const uint32_t MaxVertices = MaxCubes * 24;
        static const uint32_t MaxIndices = MaxCubes * 36;

        uint32_t m_CubeIndexCount = 0;

        //QuadVertex CubeVertexPositions[4]; switch to this once cube works
        QuadVertex CubeVertexPositions[24];

        struct CameraData
        {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
    };

    static Renderer3DData s_Data;
    
    void Renderer::Init(Window* window)
    {
        VK_PROFILE_FUNCTION();
        
        RendererAPI::Config config;
        config.window = window->getWindow();
        config.width = 800;
        config.height = 600;

        RenderCommand::SetConfig(config); // Provide config to RenderCommand
        RenderCommand::Init(); // RenderCommand creates and initializes RendererAPI instance
        
        RenderCommand::OnViewportSizeChange(Extent2D{800, 600}); // Ensures VulkanRendererAPI and your logic are in sync

        auto MeshShader = GetShaderLibrary().Load("MeshShader", "GraphicsMeshShader.slang");
        
        WatchShaderFiles();

        // Cube indices
        uint32_t* cubeIndices = new uint32_t[s_Data.MaxIndices];
        uint32_t offset = 0;
        for (int i = 0; i < s_Data.MaxIndices; i +=6)
        {
            cubeIndices[i + 0] = offset + 0;
            cubeIndices[i + 1] = offset + 1;
            cubeIndices[i + 2] = offset + 2;
            
            cubeIndices[i + 3] = offset + 2;
            cubeIndices[i + 4] = offset + 3;
            cubeIndices[i + 5] = offset + 0;

            offset += 4;
        }
        m_MeshIndexBuffer.reset(IndexBuffer::Create(s_Data.MaxIndices * sizeof(uint32_t)));
        m_MeshIndexBuffer->Upload(cubeIndices, s_Data.MaxIndices * sizeof(uint32_t));
        
        delete[] cubeIndices;
        
        // Front face - base pink
        s_Data.CubeVertexPositions[0] = {{-0.5f, -0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[1] = {{ 0.5f, -0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[2] = {{ 0.5f,  0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[3] = {{-0.5f,  0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {0,1}, 0.0f, -1};

        // Back face - slightly darker
        s_Data.CubeVertexPositions[4] = {{ 0.5f, -0.5f, -0.5f}, 0, {0.9f, 0.45f, 0.75f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[5] = {{-0.5f, -0.5f, -0.5f}, 0, {0.9f, 0.45f, 0.75f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[6] = {{-0.5f,  0.5f, -0.5f}, 0, {0.9f, 0.45f, 0.75f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[7] = {{ 0.5f,  0.5f, -0.5f}, 0, {0.9f, 0.45f, 0.75f, 1.0f}, {0,1}, 0.0f, -1};

        // Left face - slightly brighter
        s_Data.CubeVertexPositions[8]  = {{-0.5f, -0.5f, -0.5f}, 0, {1.1f, 0.55f, 0.85f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[9]  = {{-0.5f, -0.5f,  0.5f}, 0, {1.1f, 0.55f, 0.85f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[10] = {{-0.5f,  0.5f,  0.5f}, 0, {1.1f, 0.55f, 0.85f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[11] = {{-0.5f,  0.5f, -0.5f}, 0, {1.1f, 0.55f, 0.85f, 1.0f}, {0,1}, 0.0f, -1};

        // Right face - base pink
        s_Data.CubeVertexPositions[12] = {{ 0.5f, -0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[13] = {{ 0.5f, -0.5f, -0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[14] = {{ 0.5f,  0.5f, -0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[15] = {{ 0.5f,  0.5f,  0.5f}, 0, {1.0f, 0.5f, 0.8f, 1.0f}, {0,1}, 0.0f, -1};

        // Top face - slightly darker
        s_Data.CubeVertexPositions[16] = {{-0.5f,  0.5f,  0.5f}, 0, {0.95f, 0.48f, 0.77f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[17] = {{ 0.5f,  0.5f,  0.5f}, 0, {0.95f, 0.48f, 0.77f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[18] = {{ 0.5f,  0.5f, -0.5f}, 0, {0.95f, 0.48f, 0.77f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[19] = {{-0.5f,  0.5f, -0.5f}, 0, {0.95f, 0.48f, 0.77f, 1.0f}, {0,1}, 0.0f, -1};

        // Bottom face - slightly brighter
        s_Data.CubeVertexPositions[20] = {{-0.5f, -0.5f, -0.5f}, 0, {1.05f, 0.52f, 0.82f, 1.0f}, {0,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[21] = {{ 0.5f, -0.5f, -0.5f}, 0, {1.05f, 0.52f, 0.82f, 1.0f}, {1,0}, 0.0f, -1};
        s_Data.CubeVertexPositions[22] = {{ 0.5f, -0.5f,  0.5f}, 0, {1.05f, 0.52f, 0.82f, 1.0f}, {1,1}, 0.0f, -1};
        s_Data.CubeVertexPositions[23] = {{-0.5f, -0.5f,  0.5f}, 0, {1.05f, 0.52f, 0.82f, 1.0f}, {0,1}, 0.0f, -1};


        s_Data.m_CubeIndexCount = 36;

        BufferLayout layout =
        {
            {ShaderDataType::Float3, "Position"},
            {ShaderDataType::Int, "pad0"},
            {ShaderDataType::Float4, "Color"},
            {ShaderDataType::Float2, "TexCoord"},
            {ShaderDataType::Float, "TexIndex"},
            {ShaderDataType::Int, "EntityID"},
        };
        m_CubeVertexBuffer.reset(VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex)));
        m_CubeVertexBuffer->SetLayout(layout);

        m_sceneInfosBuffer.reset(UniformBuffer::Create(sizeof(shaderio::SceneInfo)));

        // Graphics Pipelines Creations
        uint32_t useTexture = true;
        std::vector<VanKSpecializationMapEntries> mapEntries
        {
                {.constantID = 0, .offset = 0, .size = sizeof(uint32_t)},
        };
        //dont like needed to be like this because if reloadpipeline then it craashes because data in struct is not copied fk this 
        VanKSpecializationInfo specInfo;
        specInfo.Data.resize(sizeof(uint32_t));
        std::memcpy(specInfo.Data.data(), &useTexture, sizeof(uint32_t));

        specInfo.MapEntries.push_back({
            .constantID = 0,
            .offset = 0,
            .size = sizeof(uint32_t)
        });
        
        VanKPipelineShaderStageCreateInfo ShaderStageCreateInfo
        {
            .VanKShader = MeshShader,
            .specializationInfo = specInfo
        };

        VanKPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo
        {
            .VanKBuffer = m_CubeVertexBuffer.get()
        };

        VanKPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo
        {
            .VanKPrimitive = VanK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST // make it shorter maybe ??
        };

        VanKPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo
        {
            .VanKPolygon = VanK_POLYGON_MODE_FILL,
            .VanKCullMode = VanK_CULL_MODE_NONE,
            .VanKFrontFace = VanK_FRONT_FACE_COUNTER_CLOCKWISE,
        };
        
        VanKGraphicsPipelineSpecification graphicsPipelineSpecification
        {
            .ShaderStageCreateInfo = ShaderStageCreateInfo,
            .VertexInputStateCreateInfo = VertexInputStateCreateInfo,
            .InputAssemblyStateCreateInfo = InputAssemblyStateCreateInfo,
            .RasterizationStateCreateInfo = RasterizerStateCreateInfo,
        };

        m_GraphicsMeshPipelineSpecification = graphicsPipelineSpecification;

        m_GraphicsMeshPipeline = RenderCommand::createGraphicsPipeline(graphicsPipelineSpecification);

        VanKSamplerCreateInfo samplerInfo{
            .magFilter = VANK_GPU_FILTER_LINEAR,
            .minFilter = VANK_GPU_FILTER_LINEAR,
            .mipmapMode = VANK_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .addressModeU = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .addressModeV = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .addressModeW = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .maxLod = VANK_GPU_LOD_CLAMP_NONE,
        };

        Renderer2D::m_sampler = Sampler::Create(samplerInfo);

        Renderer2D::m_whiteTexture = TextureImporter::LoadTexture2D("");//change this idk will see
        
        Renderer2D::Init();
    }

    void Renderer::Shutdown()
    {
        VK_PROFILE_FUNCTION();
        
        RenderCommand::waitForGraphicsQueueIdle();
        RenderCommand::DestroyAllPipelines();
        
        GetShaderLibrary().ShutdownAll();
        
        m_sceneInfosBuffer.reset();
        m_MeshIndexBuffer.reset();
        m_CubeVertexBuffer.reset();
        
        Renderer2D::Shutdown();
    }

    void Renderer::BeginSubmit()
    {
        VK_PROFILE_FUNCTION();
        
        cmd = RenderCommand::BeginCommandBuffer();
        if (!cmd)
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());

        Renderer2D::BeginSubmit(cmd);
    }

    void Renderer::EndSubmit()
    {
        VK_PROFILE_FUNCTION();
        
        RenderCommand::endFrame(cmd);
        
        Renderer2D::EndSubmit();
    }

    void Renderer::BeginScene(const EditorCamera& camera)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetViewProjection();
        sceneInfos.MatrixTransform = viewProj;

        m_sceneInfosBuffer->Update(cmd, &sceneInfos, sizeof(shaderio::SceneInfo));
        RenderCommand::BindUniformBuffer(cmd, VanKPipelineBindPoint::Graphics, m_sceneInfosBuffer.get(),
                                         shaderio::LSetScene, shaderio::LBindSceneInfo, 0);

        //Renderer2D::BeginScene(camera);
    }

    void Renderer::BeginScene(const Camera& camera, const glm::mat4& transform)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

        sceneInfos.MatrixTransform = viewProj;

        m_sceneInfosBuffer->Update(cmd, &sceneInfos, sizeof(shaderio::SceneInfo));
        RenderCommand::BindUniformBuffer(cmd, VanKPipelineBindPoint::Graphics, m_sceneInfosBuffer.get(),
                                         shaderio::LSetScene, shaderio::LBindSceneInfo, 0);

        //Renderer2D::BeginScene(camera, transform);
    }

    void Renderer::FlushBatch()
    {
        VK_PROFILE_FUNCTION();

        DrawFrame();
    }

    void Renderer::EndScene()
    {
        FlushBatch();
        
        /*Renderer2D::EndScene();*/
    }

    void Renderer::RecordGraphicCommands(VanKCommandBuffer cmd)
    {
        VK_PROFILE_FUNCTION();
        
        shaderio::PushConstant pushValues{};

        std::vector<VanKColorTargetInfo> colorAttachments;

        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R8G8B8A8_UNORM, VanK_LOADOP_CLEAR, VanK_STOREOP_STORE,
                                      VanK_FColor{.f = {0.2f, 0.2f, 0.3f, 1.0f}});
        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R32_INT, VanK_LOADOP_CLEAR, VanK_STOREOP_STORE,
                                      VanK_FColor{.i = {-1}});

        VanKDepthStencilTargetInfo depthStencilAttachment = {
            .loadOp = VanK_LOADOP_CLEAR, .storeOp = VanK_STOREOP_STORE, .clearColor = VanK_FColor{1.0f, 0}
        };

        // Upload Mesh data to GPU
        m_CubeVertexBuffer->Upload(s_Data.CubeVertexPositions, sizeof(s_Data.CubeVertexPositions));

        RenderCommand::BeginRendering(cmd, colorAttachments.data(), colorAttachments.size(), depthStencilAttachment);

        VanKViewport m_viewPort = {0, 0, m_ViewportWidth, m_ViewportHeight, 0, 1};
        RenderCommand::SetViewport(cmd, 1, m_viewPort);

        VankRect m_vankRect = {0, 0, (uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight};
        RenderCommand::SetScissor(cmd, 1, m_vankRect);

        RenderCommand::BindVertexBuffer(cmd, 0, *m_CubeVertexBuffer, 1);

        RenderCommand::BindIndexBuffer(cmd, *m_MeshIndexBuffer, VanKIndexElementSize::Uint32);
        
        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Graphics, m_GraphicsMeshPipeline);
        //notused??
        pushValues.color = glm::vec3(0, 0, 1);
        RenderCommand::PushConstants(cmd, VanKGraphics, 0, &pushValues, sizeof(shaderio::PushConstant));

        /*TextureSamplerBinding textureSamplerBinding;
        textureSamplerBinding.texture = m_texture.get();
        textureSamplerBinding.sampler = m_sampler.get();*/

        RenderCommand::BindFragmentSamplers(cmd, NULL, nullptr, NULL); //right now bindless
        
        RenderCommand::DrawIndexed(cmd, s_Data.m_CubeIndexCount, 1, 0, 0, 0);

        RenderCommand::EndRendering(cmd);
    }

    SDL_AppResult Renderer::DrawFrame()
    {
        VK_PROFILE_FUNCTION();
        
        if (s_IsPipelineReloadFinished.exchange(false))
        {
            IsShaderReloadFinished = false;
            if (s_ShaderWatcher.empty())
                WatchShaderFiles();
            
            EndSubmit();
            ReloadGraphicPipeline();
            BeginSubmit();
            return SDL_APP_CONTINUE;
        }
        
        RecordGraphicCommands(cmd);
        
        Renderer2D::recordComputeCommands(cmd);
        Renderer2D::recordGraphicCommands(cmd);
        Renderer2D::recordComputeCommandsCircles(cmd);
        Renderer2D::recordGraphicCommandsCircles(cmd);
        Renderer2D::recordGraphicCommandsLine(cmd);
        Renderer2D::recordGraphicCommandsText(cmd);
        
        return SDL_APP_CONTINUE;
    }
}
