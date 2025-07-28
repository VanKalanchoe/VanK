// No implementation needed, all methods are inline and static
#include "Renderer2D.h"

#include <filesystem>

#include "../../../../VanK-Editor/src/EditorLayer.h"
#include "VanK/Core/Log.h"

#include <imgui_internal.h>
#include <iostream>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

#include "Buffer.h"
#include "imgui.h"
#include "Shader.h"
#include "Texture.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

#include "../Vendor/FileWatch/FileWatch.h"

namespace VanK
{
    std::vector<shaderio::QuadInstance> m_QuadInstanceBuffer; // Store multiple quads
    std::vector<shaderio::CircleInstance> m_CircleInstanceBuffer; // Store multiple circles
    std::vector<shaderio::LineVertex> m_LineInstanceBuffer; // Store multiple circles
    struct Renderer2DData
    {
        const uint32_t MaxQuads = 10000;
        const uint32_t MaxVertices = MaxQuads * 4;
        const uint32_t MaxIndices = MaxQuads * 6;
    };

    static Renderer2DData s_data;
    static Renderer2D::Statistics Stats;
    
    Extent2D m_CurrentViewportSize;

    std::shared_ptr<Texture2D> m_whiteTexture = nullptr;
    std::shared_ptr<Texture2D> m_texture = nullptr;
    std::shared_ptr<Texture2D> m_texture2 = nullptr;
    std::shared_ptr<Texture2D> m_texture3 = nullptr;
    std::shared_ptr<Texture2D> m_texture4 = nullptr;

    std::unique_ptr<VertexBuffer> m_VertexBuffer;
    std::unique_ptr<VertexBuffer> m_CircleVertexBuffer;
    std::unique_ptr<VertexBuffer> m_LineVertexBuffer;
    std::unique_ptr<IndexBuffer> m_IndexBuffer;
    std::unique_ptr<UniformBuffer> m_sceneInfoBuffer;
    /*std::unique_ptr<StorageBuffer> m_pointsBuffer;*/
    std::unique_ptr<StorageBuffer> m_storageBuffer;
    std::unique_ptr<StorageBuffer> m_CircleStorageBuffer;
    std::unique_ptr<TransferBuffer> m_transferBuffer;
    std::unique_ptr<TransferBuffer> m_transferBufferCircle;

    std::unique_ptr<Shader> m_Shader;
    ShaderLibrary m_ShaderLibrary;
    static std::vector<std::unique_ptr<filewatch::FileWatch<std::string>>> s_ShaderWatchers;

    void Renderer2D::Init(Window* window)
    {
        std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
        VK_PROFILE_FUNCTION();
        /*RenderCommand::SetWindowHandle(window->getWindow());*/
        Renderer2D::window = window;
        initRenderer();
    }

    void Renderer2D::BeginSubmit()
    {
        VK_PROFILE_FUNCTION();
        cmd = RenderCommand::BeginCommandBuffer();
        if (!cmd)
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
    }

    void Renderer2D::EndSubmit()
    {
        VK_PROFILE_FUNCTION();
        RenderCommand::endFrame(cmd);
        m_QuadInstanceBuffer.clear();
        m_CircleInstanceBuffer.clear();
        m_LineInstanceBuffer.clear();
        Stats.QuadCount = 0;
    }

    void Renderer2D::Shutdown()
    {
        VK_PROFILE_FUNCTION();
        shutdownRenderer();
    }

    void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

        /*SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(viewProj), sizeof(glm::mat4));*/

        /*float uniformData[2] = {nearPlane, farPlane};*/
        /*SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);*/

        // Updating the data for the frame
        shaderio::SceneInfo sceneInfo{};
        sceneInfo.MatrixTransform = viewProj;
        /*sceneInfo.nearPlane = nearPlane;
        sceneInfo.farPlane = farPlane;*/

        m_sceneInfoBuffer->Update(cmd, &sceneInfo, sizeof(shaderio::SceneInfo));
        RenderCommand::BindUniformBuffer(cmd, VanKPipelineBindPoint::Graphics, m_sceneInfoBuffer.get(),
                                         shaderio::LSetScene, shaderio::LBindSceneInfo, 0);
    }

    void Renderer2D::BeginScene(const EditorCamera& camera)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetViewProjection();
        /*std::cout << "MatrixTransform:\n";*/
        /*for (int i = 0; i < 4; ++i)
            std::cout << viewProj[i][0] << " " << viewProj[i][1] << " " << viewProj[i][2] << " " << viewProj[i][3] << std::endl;*/
        /*SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(viewProj), sizeof(glm::mat4));*/

        /*float uniformData[2] = {nearPlane, farPlane};*/
        /*SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);*/

        shaderio::SceneInfo sceneInfo{};
        sceneInfo.MatrixTransform = viewProj;
        /*sceneInfo.nearPlane = camera.m_NearClip;
        sceneInfo.farPlane = camera.m_FarClip;*/

        m_sceneInfoBuffer->Update(cmd, &sceneInfo, sizeof(shaderio::SceneInfo));
        RenderCommand::BindUniformBuffer(cmd, VanKPipelineBindPoint::Graphics, m_sceneInfoBuffer.get(),
                                         shaderio::LSetScene, shaderio::LBindSceneInfo, 0);
    }

    // Global declarations
    int index = 0;

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        VK_PROFILE_FUNCTION();
        glm::mat4 cam = camera.GetViewProjectionMatrix();
        /*SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(cam), sizeof(glm::mat4));*/

        /*float uniformData[2] = {camera.GetNearPlane(), camera.GetFarPlane()};*/
        /*SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);*/

        shaderio::SceneInfo sceneInfo{};
        sceneInfo.MatrixTransform = cam;
        sceneInfo.nearPlane = camera.GetNearPlane();
        sceneInfo.farPlane = camera.GetFarPlane();

        m_sceneInfoBuffer->Update(cmd, &sceneInfo, sizeof(shaderio::SceneInfo));
        RenderCommand::BindUniformBuffer(cmd, VanKPipelineBindPoint::Graphics, m_sceneInfoBuffer.get(),
                                         shaderio::LSetScene, shaderio::LBindSceneInfo, 0);
    }

    void Renderer2D::FlushBatch()
    {
        VK_PROFILE_FUNCTION();

        drawFrame();
    }

    void Renderer2D::EndScene()
    {
        FlushBatch();
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition,
                              glm::vec2 tileSize,
                              glm::vec2 tileMultiplier, glm::vec2 atlasSize)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, color, tilePosition, tileSize,
                 tileMultiplier, atlasSize);
    }

    //atlas
    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition,
                              glm::vec2 tileSize,
                              glm::vec2 tileMultiplier, glm::vec2 atlasSize)
    {
        /*if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
        {
            /*FlushBatch();
            m_QuadInstanceBuffer.clear();#1#
            VK_CORE_INFO("to many draws make spritecount bigger");
            //make sprite count dynamic and delete this shit
        }

        QuadInstance quad;
        quad.position = position;
        quad.rotation = rotation;
        quad.scale = scale;
        quad.size = size;
        quad.color = color;
        quad.tilePosition = tilePosition;
        quad.tileSize = tileSize;
        quad.tileMultiplier = tileMultiplier;
        quad.atlasSize = atlasSize;

        m_QuadInstanceBuffer.emplace_back(quad);

        Stats.QuadCount++;*/
    }

    //no image
    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, int entityID)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, color, entityID);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, int entityID)
    {
        /*if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
        {
            /*FlushBatch();
            m_QuadInstanceBuffer.clear();#1#
            VK_CORE_INFO("to many draws make spritecount bigger");
        }*/

        /*TextureMap["whiteSurface"] = TextureMap[TextureName];*/

        shaderio::QuadInstance quad;
        quad.Position = position;
        quad.Rotation = rotation;
        quad.Size = size;
        quad.Scale = scale;
        quad.TextureID = m_whiteTexture->GetTextureIndex();
        quad.Color = color;
        quad.tilePosition = {0.0f, 0.0f};
        quad.tileSize = {0.0f, 0.0f};
        quad.tileMultiplier = {0.0f, 0.0f};
        quad.atlasSize = {0.0f, 0.0f};
        quad.TilingFactor = 1.0f;
        quad.EntityID = entityID;

        m_QuadInstanceBuffer.emplace_back(quad);

        Stats.QuadCount++;
    }

    void Renderer2D::DrawCircle(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                                const glm::vec3& rotation, const glm::vec4& color, float thickness, float fade,
                                int entityID)
    {
        DrawCircle(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, color, thickness, fade, entityID);
    }

    void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                                const glm::vec3& rotation, const glm::vec4& color, float thickness, float fade,
                                int entityID)
    {
        shaderio::CircleInstance circle;
        circle.WorldPosition = position;
        circle.LocalPosition = position;
        circle.Rotation = rotation;
        circle.Size = size;
        circle.Scale = scale;
        circle.Color = color;
        circle.Thickness = thickness;
        circle.Fade = fade;
        circle.EntityID = entityID;

        m_CircleInstanceBuffer.emplace_back(circle);

        Stats.QuadCount++;
    }

    void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int EntityID)
    {
        shaderio::LineVertex lineVertex;
        lineVertex.Position = p0;
        lineVertex.Color = color;
        lineVertex.EntityID = EntityID;

        m_LineInstanceBuffer.emplace_back(lineVertex);

        shaderio::LineVertex lineVertex2;
        lineVertex2.Position = p1;
        lineVertex2.Color = color;
        lineVertex2.EntityID = EntityID;

        m_LineInstanceBuffer.emplace_back(lineVertex2);

        Stats.QuadCount +=2;
    }

    void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int EntityID)
    {
        glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5, position.y - size.y * 0.5, position.z);
        glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5, position.y - size.y * 0.5, position.z);
        glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5, position.y + size.y * 0.5, position.z);
        glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5, position.y + size.y * 0.5, position.z);
        
        DrawLine(p0, p1, color, EntityID);
        DrawLine(p1, p2, color, EntityID);
        DrawLine(p2, p3, color, EntityID);
        DrawLine(p3, p0, color, EntityID);
    }

    void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int EntityID)
    {
        glm::vec3 p0 = transform * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
        glm::vec3 p1 = transform * glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f);
        glm::vec3 p2 = transform * glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f);
        glm::vec3 p3 = transform * glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f);
        
        DrawLine(p0, p1, color, EntityID);
        DrawLine(p1, p2, color, EntityID);
        DrawLine(p2, p3, color, EntityID);
        DrawLine(p3, p0, color, EntityID);
    }

    void Renderer2D::DrawRect(TransformComponent tc, SpriteRendererComponent src, int entityID)
    {
        //maybe use the computeshader of quads and then copy that vertbuffer to linebuffer ? idk
        glm::vec3 localCorners[4] = {
            {-0.5f, -0.5f, 0.0f},
            { 0.5f, -0.5f, 0.0f},
            { 0.5f,  0.5f, 0.0f},
            {-0.5f,  0.5f, 0.0f}
        };

        glm::vec3 scaledSize = glm::vec3(tc.Size.x * tc.Scale.x, tc.Size.y * tc.Scale.y, 1.0f);

        for (int i = 0; i < 4; ++i)
            localCorners[i] *= scaledSize;

        float angle = tc.Rotation.z;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, {0, 0, 1});

        glm::vec3 lineVertices[4];
        for (int i = 0; i < 4; ++i)
            lineVertices[i] = glm::vec3(rotation * glm::vec4(localCorners[i], 1.0f)) + tc.Position;

        /*DrawQuad(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.Color, entityID);*/

        DrawLine(lineVertices[0], lineVertices[1], src.Color, entityID);
        DrawLine(lineVertices[1], lineVertices[2], src.Color, entityID);
        DrawLine(lineVertices[2], lineVertices[3], src.Color, entityID);
        DrawLine(lineVertices[3], lineVertices[0], src.Color, entityID);
    }

    //image
    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const Ref<Texture2D>& texture, float TilingFactor,
                              const glm::vec4& color, int entityID)
    {
        std::cout << "halbe" << std::endl;
        //todo
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, texture, TilingFactor, color,
                 entityID);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const Ref<Texture2D>& texture, float TilingFactor,
                              const glm::vec4& color, int entityID)
    {
        /*if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
        {
            /*FlushBatch();
            m_QuadInstanceBuffer.clear();#1#
            VK_CORE_INFO("to many draws make spritecount bigger");
        }*/

        shaderio::QuadInstance quad;
        quad.Position = position;
        quad.Rotation = rotation;
        quad.Size = size;
        quad.Scale = scale;
        quad.TextureID = texture->GetTextureIndex();
        quad.Color = color;
        quad.tilePosition = {0.0f, 0.0f};
        quad.tileSize = {0.0f, 0.0f};
        quad.tileMultiplier = {0.0f, 0.0f};
        quad.atlasSize = {0.0f, 0.0f};
        quad.TilingFactor = TilingFactor;
        quad.EntityID = entityID;

        /*TextureMap["whiteSurface"] = TextureMap[TextureName];*/

        m_QuadInstanceBuffer.emplace_back(quad);
        /*std::cout  << "halbe bilbel " << m_QuadInstanceBuffer[0].TextureID << std::endl;*/
        Stats.QuadCount++; //wird nicht aufgerufen die method schau mal warum nach
    }

    void Renderer2D::DrawSprite(TransformComponent tc, SpriteRendererComponent src, int entityID)
    {
        if (src.Texture)
        {
            DrawQuad(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.Texture, src.TilingFactor, src.Color, entityID);
        }
        else
        {
            DrawQuad(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.Color, entityID);
        }
    }

    void Renderer2D::DrawCircle(TransformComponent tc, CircleRendererComponent src, int entityID)
    {
        DrawCircle(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.Color, src.Thickness, src.Fade, entityID);
    }

    void Renderer2D::initRenderer()
    {
        RendererAPI::Config config;
        config.window = window->getWindow();
        config.width = 800;
        config.height = 600;

        RenderCommand::SetConfig(config); // Provide config to RenderCommand
        RenderCommand::Init(); // RenderCommand creates and initializes RendererAPI instance
        
        RenderCommand::OnViewportSizeChange(Extent2D{800, 600}); // Ensures VulkanRendererAPI and your logic are in sync
        
        //once at start time name + filepath better otherwise you need to use full path name everywhere
        auto textureShader = m_ShaderLibrary.Load("textureShader", "shader.rast.slang");
        auto circleShader = m_ShaderLibrary.Load("CircleShader", "shader.CircleRast.slang");
        auto lineShader = m_ShaderLibrary.Load("LineShader", "shader.LineRast.slang");

        auto computeShader = m_ShaderLibrary.Load("computeShader", "shader.comp.slang"); //once at start time
        auto computeCircleShader = m_ShaderLibrary.Load("computeCircleShader", "shader.CircleComp.slang");
        //once at start time

        watchShaderFiles();

        BufferLayout layout =
        {
            {ShaderDataType::Float4, "Position"},
            {ShaderDataType::Float2, "Texcoord"},
            {ShaderDataType::Float2, "pad1"},
            {ShaderDataType::Float4, "Color"},
            {ShaderDataType::Int, "EntityID"},
            {ShaderDataType::Int, "TextureID"},
            {ShaderDataType::Float2, "pad2"},
        };

        /*m_VertexBuffer.reset(VertexBuffer::Create(std::span<const shaderio::Vertex>(s_vertices)));*/
        m_VertexBuffer.reset(VertexBuffer::Create(s_data.MaxVertices * sizeof(shaderio::Vertex)));
        /*m_VertexBuffer->Upload(s_vertices.data(), s_vertices.size() * sizeof(shaderio::Vertex));*/
        m_VertexBuffer->SetLayout(layout);
        //if index or vertex buffer needs resize i should add barrier to check if they are finished
        BufferLayout Circlelayout =
        {
            {ShaderDataType::Float4, "WorldPosition"},
            {ShaderDataType::Float3, "LocalPosition"},
            {ShaderDataType::Float, "pad0"},
            {ShaderDataType::Float4, "Color"},
            {ShaderDataType::Float, "Thickness"},
            {ShaderDataType::Float, "Fade"},
            {ShaderDataType::Int, "EntityID"},
            {ShaderDataType::Float, "pad1"},
        };
        //seperate vertex buffer i think thats the problem why it crashes
        m_CircleVertexBuffer.reset(VertexBuffer::Create(s_data.MaxVertices * sizeof(shaderio::CircleVertex)));
        m_CircleVertexBuffer->SetLayout(Circlelayout);

        //if index or vertex buffer needs resize i should add barrier to check if they are finished
        BufferLayout Linelayout =
        {
            {ShaderDataType::Float3, "Position"},
            {ShaderDataType::Float, "pad"},
            {ShaderDataType::Float4, "Color"},
            {ShaderDataType::Int, "EntityID"},
            {ShaderDataType::Float3, "pad1"},
        };
        //seperate vertex buffer i think thats the problem why it crashes
        m_LineVertexBuffer.reset(VertexBuffer::Create(s_data.MaxVertices * sizeof(shaderio::LineVertex)));
        m_LineVertexBuffer->SetLayout(Linelayout);

        /*m_IndexBuffer.reset(IndexBuffer::Create(std::span<const uint32_t>(s_indices), 0));*/
        m_IndexBuffer.reset(IndexBuffer::Create(s_data.MaxIndices * sizeof(uint32_t)));
        static std::vector<uint32_t> s_indices(s_data.MaxIndices);
        for (Uint32 i = 0, j = 0; i < s_data.MaxIndices; i += 6, j += 4)
        {
            s_indices[i + 0] = j + 0;
            s_indices[i + 1] = j + 1;
            s_indices[i + 2] = j + 2;
            
            s_indices[i + 3] = j + 2;
            s_indices[i + 4] = j + 3;
            s_indices[i + 5] = j + 0;
        }
        m_IndexBuffer->Upload(s_indices.data(), s_data.MaxIndices * sizeof(uint32_t));

        m_sceneInfoBuffer.reset(UniformBuffer::Create(sizeof(shaderio::SceneInfo)));

        m_transferBuffer.reset(TransferBuffer::Create(s_data.MaxQuads * sizeof(shaderio::QuadInstance),
                                                      VanKTransferBufferUsageUpload));

        m_transferBufferCircle.reset(TransferBuffer::Create(s_data.MaxQuads * sizeof(shaderio::CircleInstance),
                                                      VanKTransferBufferUsageUpload));
        
        m_storageBuffer.reset(StorageBuffer::Create(s_data.MaxQuads * sizeof(shaderio::QuadInstance)));
        m_CircleStorageBuffer.reset(StorageBuffer::Create(s_data.MaxQuads * sizeof(shaderio::CircleInstance)));
        //automate usage flags instead of hardcoding in vulkanbuffer

        // graphics pipelines creations
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
            .VanKShader = textureShader,
            .specializationInfo = specInfo
        };

        VanKPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo
        {
            .VanKBuffer = m_VertexBuffer.get()
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

        m_graphicsTexturePipelineSpecification = graphicsPipelineSpecification;

        textureGraphicsPipeline = RenderCommand::createGraphicsPipeline(graphicsPipelineSpecification);
        
        graphicsPipelineSpecification.ShaderStageCreateInfo.VanKShader = circleShader;
        graphicsPipelineSpecification.ShaderStageCreateInfo.specializationInfo.reset();
        graphicsPipelineSpecification.VertexInputStateCreateInfo.VanKBuffer = m_CircleVertexBuffer.get();

        m_graphicsCirclePipelineSpecification = graphicsPipelineSpecification;

        circleGraphicsPipeline = RenderCommand::createGraphicsPipeline(graphicsPipelineSpecification);

        graphicsPipelineSpecification.ShaderStageCreateInfo.VanKShader = lineShader;
        graphicsPipelineSpecification.VertexInputStateCreateInfo.VanKBuffer = m_LineVertexBuffer.get();
        graphicsPipelineSpecification.InputAssemblyStateCreateInfo.VanKPrimitive = VanK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        m_lineGraphicsPipelineSpecification = graphicsPipelineSpecification;
        //make lines in shader instead of using line list ways better
        lineGraphicsPipeline = RenderCommand::createGraphicsPipeline(graphicsPipelineSpecification);

        // Compute Pipelines creations
        VanKComputePipelineCreateInfo ComputePipelineCreateInfo
        {
            .VanKShader = computeShader,
        };
        
        VanKComputePipelineSpecification computePipelineSpecification
        {
            .ComputePipelineCreateInfo = ComputePipelineCreateInfo
        };

        m_computeTexturePipelineSpecification = computePipelineSpecification;
        
        textureComputePipeline = RenderCommand::createComputeShaderPipeline(computePipelineSpecification);

        computePipelineSpecification.ComputePipelineCreateInfo.VanKShader = computeCircleShader;

        circleComputePipeline = RenderCommand::createComputeShaderPipeline(computePipelineSpecification);

        VanKSamplerCreateInfo samplerInfo{
            .magFilter = VANK_GPU_FILTER_LINEAR,
            .minFilter = VANK_GPU_FILTER_LINEAR,
            .mipmapMode = VANK_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .addressModeU = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .addressModeV = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .addressModeW = VANK_GPU_SAMPLERADDRESSMODE_REPEAT,
            .maxLod = VANK_GPU_LOD_CLAMP_NONE,
        };

        m_sampler = Sampler::Create(samplerInfo);

        m_whiteTexture = Texture2D::Create("", m_sampler);
        /*
        m_texture = Texture2D::Create("image1.jpg", m_sampler);
        m_texture2 = Texture2D::Create("image2.jpg", m_sampler);
        m_texture3 = Texture2D::Create("ChernoLogo.png", m_sampler);
        m_texture4 = Texture2D::Create("char2.png", m_sampler);
        */

        //imgui fix currently all textures are together swapped not individual i guess

        // currently im not deleting images that are created in components i think neeed to check if descturctor deletes or not ?
        // second pipeline internally creates another something and that gets not killed i assume
    }

    void Renderer2D::shutdownRenderer()
    {
        RenderCommand::waitForGraphicsQueueIdle();
        std::cout << "Shutting down renderer" << '\n';
        m_ShaderLibrary.ShutdownAll();
        m_sceneInfoBuffer.reset(); //maybe bufferlibrary like shader to kill all at once
        /*m_pointsBuffer.reset();*/
        m_VertexBuffer.reset();
        m_CircleVertexBuffer.reset();
        m_LineVertexBuffer.reset();

        m_IndexBuffer.reset();
        
        m_storageBuffer.reset();
        m_CircleStorageBuffer.reset();
        
        m_transferBuffer.reset();
        m_transferBufferCircle.reset();
    }

    void Renderer2D::watchShaderFiles()
    {
        for (const std::string& path : m_ShaderLibrary.GetAllShaderPaths())
        {
            s_ShaderWatchers.emplace_back(std::make_unique<filewatch::FileWatch<std::string>>(path,
                [](const std::string& file, const filewatch::Event change_type)
                {
                    if (change_type == filewatch::Event::modified)
                    {
                        std::cout << "[FileWatcher] Shader file changed: " << file << '\n';
                        Renderer2D::s_NeedsPipelineReload = true;
                    }
                }));
        }
    }

    void Renderer2D::useImGui()
    {
        bool old = m_useImGui;
        m_useImGui = !m_useImGui;

        if (old && !m_useImGui)
            m_forceViewportResize = true; // ImGui just got turned off
    }

    void Renderer2D::rendererEvent(SDL_Event* event)
    {
        ImGui_ImplSDL3_ProcessEvent(event);
    }

    void Renderer2D::OnViewportSizeChange(const Extent2D& newSize)
    {
        if (newSize.width != lastViewportSize.width || newSize.height != lastViewportSize.height)
        {
            /*lastViewportSize = newSize;
            SceneWidth = newSize.width;
            SceneHeight = newSize.height;
            RenderCommand::OnViewportSizeChange(newSize);*/
        }
    }

    void Renderer2D::reloadGraphicsPipeline()
    {
        //maybe not destory all pipeplines only the one needed to be reloaded
        RenderCommand::waitForGraphicsQueueIdle();
        RenderCommand::destroyGraphicsPipeline();
        RenderCommand::destroyComputePipeline();
        
        m_ShaderLibrary.ShutdownAll();
        auto textureShader = m_ShaderLibrary.Load("textureShader", "shader.rast.slang");
        auto CircleShader = m_ShaderLibrary.Load("CircleShader", "shader.CircleRast.slang");
        auto LineShader = m_ShaderLibrary.Load("LineShader", "shader.LineRast.slang");

        auto computeShader = m_ShaderLibrary.Load("computeShader", "shader.comp.slang"); //once at start time
        auto computeCircleShader = m_ShaderLibrary.Load("computeCircleShader", "shader.CircleComp.slang");

        //graphics
        m_graphicsTexturePipelineSpecification.ShaderStageCreateInfo.VanKShader = textureShader;
        
        textureGraphicsPipeline = RenderCommand::createGraphicsPipeline(m_graphicsTexturePipelineSpecification);

        m_graphicsCirclePipelineSpecification.ShaderStageCreateInfo.VanKShader = CircleShader;

        circleGraphicsPipeline = RenderCommand::createGraphicsPipeline(m_graphicsCirclePipelineSpecification);

        m_lineGraphicsPipelineSpecification.ShaderStageCreateInfo.VanKShader = LineShader;

        lineGraphicsPipeline = RenderCommand::createGraphicsPipeline(m_lineGraphicsPipelineSpecification);

        //compute
        m_computeTexturePipelineSpecification.ComputePipelineCreateInfo.VanKShader = computeShader;
        
        textureComputePipeline = RenderCommand::createComputeShaderPipeline(m_computeTexturePipelineSpecification);

        m_computeTexturePipelineSpecification.ComputePipelineCreateInfo.VanKShader = computeCircleShader;
        
        circleComputePipeline = RenderCommand::createComputeShaderPipeline(m_computeTexturePipelineSpecification);
    }
    
    void Renderer2D::recordComputeCommands(VanKCommandBuffer cmd)
    {
        shaderio::PushConstantCompute pushValues{};
        pushValues.numVertex = m_QuadInstanceBuffer.size();
        RenderCommand::PushConstants(cmd, VanKCompute, 0, &pushValues, sizeof(shaderio::PushConstantCompute));
        //change internal so it checks if usage is storage or not
        //and remove uniform bind sdl doesnt provide that i think they interanly create a uniform buffer when push constant used like pushuniform vertexdata frag compute
        /*if (!m_QuadInstanceBuffer.empty()) m_storageBuffer->Update(cmd, m_QuadInstanceBuffer.data(), m_QuadInstanceBuffer.size() * sizeof(shaderio::QuadInstance));*/

        shaderio::QuadInstance* dataPtr = static_cast<shaderio::QuadInstance*>(m_transferBuffer->MapTransferBuffer());
        //instead of for loop use memcpy
        /*for (Uint32 i = 0; i < m_QuadInstanceBuffer.size(); i++)
        {
            dataPtr[i].Position = m_QuadInstanceBuffer[i].Position;
            dataPtr[i].Rotation = m_QuadInstanceBuffer[i].Rotation;
            dataPtr[i].Size = m_QuadInstanceBuffer[i].Size;
            dataPtr[i].Scale = m_QuadInstanceBuffer[i].Scale;
            dataPtr[i].TextureID = m_QuadInstanceBuffer[i].TextureID;
            dataPtr[i].Color = m_QuadInstanceBuffer[i].Color;
            dataPtr[i].tilePosition = m_QuadInstanceBuffer[i].tilePosition;
            dataPtr[i].tileSize = m_QuadInstanceBuffer[i].tileSize;
            dataPtr[i].tileMultiplier = m_QuadInstanceBuffer[i].tileMultiplier;
            dataPtr[i].atlasSize = m_QuadInstanceBuffer[i].atlasSize;
            dataPtr[i].EntityID = m_QuadInstanceBuffer[i].EntityID;
        }*/
        memcpy(dataPtr, m_QuadInstanceBuffer.data(),
               (size_t)m_QuadInstanceBuffer.size() * sizeof(shaderio::QuadInstance));
        m_transferBuffer->UnMapTransferBuffer();

        m_transferBuffer->UploadToGPUBuffer(cmd, VanKTransferBufferLocation{.offset = 0}, VanKBufferRegion{
                                                .buffer = m_storageBuffer.get(), .offset = 0,
                                                .size = m_QuadInstanceBuffer.size() * sizeof(shaderio::QuadInstance)
                                            });

        VanKComputePass* computePass = RenderCommand::BeginComputePass(cmd, m_VertexBuffer.get());
        
        RenderCommand::BindStorageBuffer(cmd, VanKPipelineBindPoint::Compute, m_storageBuffer.get(), 1, 1, 0);
        RenderCommand::BindStorageBuffer(cmd, VanKPipelineBindPoint::Compute, m_VertexBuffer.get(), 1, 2, 0);

        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Compute, textureComputePipeline);

        RenderCommand::DispatchCompute(computePass, (pushValues.numVertex + 255) / 256, 1, 1);

        RenderCommand::EndComputePass(computePass);
    }

    void Renderer2D::recordGraphicCommands(VanKCommandBuffer cmd)
    {
        shaderio::PushConstant pushValues{};

        std::vector<VanKColorTargetInfo> colorAttachments;

        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R8G8B8A8_UNORM, VanK_LOADOP_CLEAR, VanK_STOREOP_STORE,
                                      VanK_FColor{.f = {0.2f, 0.2f, 0.3f, 1.0f}});
        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R32_INT, VanK_LOADOP_CLEAR, VanK_STOREOP_STORE,
                                      VanK_FColor{.i = {-1}});

        VanKDepthStencilTargetInfo depthStencilAttachment = {
            .loadOp = VanK_LOADOP_CLEAR, .storeOp = VanK_STOREOP_STORE, .clearColor = VanK_FColor{1.0f, 0}
        };

        RenderCommand::BeginRendering(cmd, colorAttachments.data(), colorAttachments.size(), depthStencilAttachment);

        VanKViewport m_viewPort = {0, 0, m_ViewportWidth, m_ViewportHeight, 0, 1};
        RenderCommand::SetViewport(cmd, 1, m_viewPort);

        VankRect m_vankRect = {0, 0, (uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight};
        RenderCommand::SetScissor(cmd, 1, m_vankRect);

        RenderCommand::BindVertexBuffer(cmd, 0, *m_VertexBuffer, 1);

        RenderCommand::BindIndexBuffer(cmd, *m_IndexBuffer, VanKIndexElementSize::Uint32);
        
        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Graphics, textureGraphicsPipeline);
        //notused??
        pushValues.color = glm::vec3(0, 0, 1);
        RenderCommand::PushConstants(cmd, VanKGraphics, 0, &pushValues, sizeof(shaderio::PushConstant));

        /*TextureSamplerBinding textureSamplerBinding;
        textureSamplerBinding.texture = m_texture.get();
        textureSamplerBinding.sampler = m_sampler.get();*/

        RenderCommand::BindFragmentSamplers(cmd, NULL, nullptr, NULL); //right now bindless
        
        RenderCommand::DrawIndexed(cmd, m_QuadInstanceBuffer.size() * 6, 1, 0, 0, 0);

        RenderCommand::EndRendering(cmd);
    }

    void Renderer2D::recordComputeCommandsCircles(VanKCommandBuffer cmd)
    {
        shaderio::PushConstantCompute pushCircleValues{};
        pushCircleValues.numVertex = m_CircleInstanceBuffer.size();
        RenderCommand::PushConstants(cmd, VanKCompute, 0, &pushCircleValues, sizeof(shaderio::PushConstantCompute));
        //change internal so it checks if usage is storage or not
        //and remove uniform bind sdl doesnt provide that i think they interanly create a uniform buffer when push constant used like pushuniform vertexdata frag compute
        /*if (!m_QuadInstanceBuffer.empty()) m_storageBuffer->Update(cmd, m_QuadInstanceBuffer.data(), m_QuadInstanceBuffer.size() * sizeof(shaderio::QuadInstance));*/

        shaderio::CircleInstance* dataPtrCircle = static_cast<shaderio::CircleInstance*>(m_transferBufferCircle->
            MapTransferBuffer());
        //instead of for loop use memcpy
        /*for (Uint32 i = 0; i < m_QuadInstanceBuffer.size(); i++)
        {
            dataPtr[i].Position = m_QuadInstanceBuffer[i].Position;
            dataPtr[i].Rotation = m_QuadInstanceBuffer[i].Rotation;
            dataPtr[i].Size = m_QuadInstanceBuffer[i].Size;
            dataPtr[i].Scale = m_QuadInstanceBuffer[i].Scale;
            dataPtr[i].TextureID = m_QuadInstanceBuffer[i].TextureID;
            dataPtr[i].Color = m_QuadInstanceBuffer[i].Color;
            dataPtr[i].tilePosition = m_QuadInstanceBuffer[i].tilePosition;
            dataPtr[i].tileSize = m_QuadInstanceBuffer[i].tileSize;
            dataPtr[i].tileMultiplier = m_QuadInstanceBuffer[i].tileMultiplier;
            dataPtr[i].atlasSize = m_QuadInstanceBuffer[i].atlasSize;
            dataPtr[i].EntityID = m_QuadInstanceBuffer[i].EntityID;
        }*/
        memcpy(dataPtrCircle, m_CircleInstanceBuffer.data(),
               (size_t)m_CircleInstanceBuffer.size() * sizeof(shaderio::CircleInstance));
        m_transferBufferCircle->UnMapTransferBuffer();

        m_transferBufferCircle->UploadToGPUBuffer(cmd, VanKTransferBufferLocation{.offset = 0}, VanKBufferRegion{
                                                .buffer = m_CircleStorageBuffer.get(), .offset = 0,
                                                .size = m_CircleInstanceBuffer.size() * sizeof(shaderio::CircleInstance)
                                            });

        VanKComputePass* computePass = RenderCommand::BeginComputePass(cmd, m_CircleVertexBuffer.get());
        
        RenderCommand::BindStorageBuffer(cmd, VanKPipelineBindPoint::Compute, m_CircleStorageBuffer.get(), 1, 3, 0);
        RenderCommand::BindStorageBuffer(cmd, VanKPipelineBindPoint::Compute, m_CircleVertexBuffer.get(), 1, 2, 0);

        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Compute, circleComputePipeline);

        RenderCommand::DispatchCompute(computePass, (pushCircleValues.numVertex + 255) / 256, 1, 1);

        RenderCommand::EndComputePass(computePass);
    }

    void Renderer2D::recordGraphicCommandsCircles(VanKCommandBuffer cmd)
    {
        shaderio::PushConstant pushValues{};

        std::vector<VanKColorTargetInfo> colorAttachments;

        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R8G8B8A8_UNORM, VanK_LOADOP_LOAD, VanK_STOREOP_STORE,
                                      VanK_FColor{.f = {0.2f, 0.2f, 0.3f, 1.0f}});
        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R32_INT, VanK_LOADOP_LOAD, VanK_STOREOP_STORE,
                                      VanK_FColor{.i = {-1}});

        VanKDepthStencilTargetInfo depthStencilAttachment = {
            .loadOp = VanK_LOADOP_CLEAR, .storeOp = VanK_STOREOP_STORE, .clearColor = VanK_FColor{1.0f, 0}
        };

        RenderCommand::BeginRendering(cmd, colorAttachments.data(), colorAttachments.size(), depthStencilAttachment);

        VanKViewport m_viewPort = {0, 0, m_ViewportWidth, m_ViewportHeight, 0, 1};
        RenderCommand::SetViewport(cmd, 1, m_viewPort);

        VankRect m_vankRect = {0, 0, (uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight};
        RenderCommand::SetScissor(cmd, 1, m_vankRect);

        RenderCommand::BindVertexBuffer(cmd, 0, *m_CircleVertexBuffer, 1);

        RenderCommand::BindIndexBuffer(cmd, *m_IndexBuffer, VanKIndexElementSize::Uint32);

        /*
        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Graphics, VanKPipelineType::GraphicsWithoutTexture);
    
        pushValues.color = glm::vec3(1, 0, 0);
        RenderCommand::PushConstants(cmd, VanKGraphics, 0, &pushValues, sizeof(shaderio::PushConstant));
    
        RenderCommand::BindFragmentSamplers(cmd, NULL, nullptr, NULL); //right now bindless
    
        RenderCommand::DrawIndexed(cmd, 6, 1, 0, 0, 0);
        */

        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Graphics, circleGraphicsPipeline);
        //notused??
        pushValues.color = glm::vec3(0, 0, 1);
        RenderCommand::PushConstants(cmd, VanKGraphics, 0, &pushValues, sizeof(shaderio::PushConstant));

        /*TextureSamplerBinding textureSamplerBinding;
        textureSamplerBinding.texture = m_texture.get();
        textureSamplerBinding.sampler = m_sampler.get();*/

        RenderCommand::BindFragmentSamplers(cmd, NULL, nullptr, NULL); //right now bindless
        /*std::cout << "Quads to draw: " << m_QuadInstanceBuffer.size() << std::endl;*/
        RenderCommand::DrawIndexed(cmd, m_CircleInstanceBuffer.size() * 6, 1, 0, 0, 0);

        RenderCommand::EndRendering(cmd);
    }

    void Renderer2D::recordGraphicCommandsLine(VanKCommandBuffer cmd)
    {
        shaderio::PushConstant pushValues{};

        std::vector<VanKColorTargetInfo> colorAttachments;

        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R8G8B8A8_UNORM, VanK_LOADOP_LOAD, VanK_STOREOP_STORE,
                                      VanK_FColor{.f = {0.2f, 0.2f, 0.3f, 1.0f}});
        colorAttachments.emplace_back(VanK_TEXTUREFORMAT_R32_INT, VanK_LOADOP_LOAD, VanK_STOREOP_STORE,
                                      VanK_FColor{.i = {-1}});

        VanKDepthStencilTargetInfo depthStencilAttachment = {
            .loadOp = VanK_LOADOP_CLEAR, .storeOp = VanK_STOREOP_STORE, .clearColor = VanK_FColor{1.0f, 0}
        };
        // Upload line data to GPU
        if (!m_LineInstanceBuffer.empty()) {
            m_LineVertexBuffer->Upload(m_LineInstanceBuffer.data(), m_LineInstanceBuffer.size() * sizeof(shaderio::LineVertex));
        }
        RenderCommand::BeginRendering(cmd, colorAttachments.data(), colorAttachments.size(), depthStencilAttachment);

        VanKViewport m_viewPort = {0, 0, m_ViewportWidth, m_ViewportHeight, 0, 1};
        RenderCommand::SetViewport(cmd, 1, m_viewPort);

        VankRect m_vankRect = {0, 0, (uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight};
        RenderCommand::SetScissor(cmd, 1, m_vankRect);

        RenderCommand::SetLineWidth(cmd, 4.0f);

        RenderCommand::BindVertexBuffer(cmd, 0, *m_LineVertexBuffer, 1);
        
        RenderCommand::BindPipeline(cmd, VanKPipelineBindPoint::Graphics, lineGraphicsPipeline);

        //notused??
        pushValues.color = glm::vec3(0, 0, 1);
        RenderCommand::PushConstants(cmd, VanKGraphics, 0, &pushValues, sizeof(shaderio::PushConstant));

        RenderCommand::BindFragmentSamplers(cmd, NULL, nullptr, NULL); //right now bindless
        
        RenderCommand::Draw(cmd, m_LineInstanceBuffer.size(), 1, 0, 0);

        RenderCommand::EndRendering(cmd);
    }

    SDL_AppResult Renderer2D::drawFrame()
    {
        if (Renderer2D::s_NeedsPipelineReload.exchange(false))
        {
            EndSubmit();
            Renderer2D::reloadGraphicsPipeline(); // Safely done in render thread
            BeginSubmit();
            return SDL_APP_CONTINUE;
        }
        
        recordComputeCommands(cmd);
        recordGraphicCommands(cmd);
        recordComputeCommandsCircles(cmd);
        recordGraphicCommandsCircles(cmd);
        recordGraphicCommandsLine(cmd);
        
        return SDL_APP_CONTINUE;
    }

    void Renderer2D::ResetStats()
    {
        std::memset(&Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats()
    {
        return Stats;
    }
}