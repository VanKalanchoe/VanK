/*#include "Renderer2D.h"

#include "../../../../VanK-Editor/src/EditorLayer.h"
#include "VanK/Core/Log.h"

namespace VanK
{
    static Renderer2D::Statistics Stats;

    SDL_GPUDevice* Renderer2D::device = nullptr;
    Window* Renderer2D::window = nullptr;
    SDL_GPUGraphicsPipeline* Renderer2D::RenderPipeline = nullptr;
    SDL_GPUGraphicsPipeline* Renderer2D::EffectPipeline = nullptr;
    SDL_GPUComputePipeline* Renderer2D::ComputePipeline = nullptr;
    SDL_GPUSampler* Renderer2D::Sampler = nullptr;
    SDL_GPUTransferBuffer* Renderer2D::SpriteComputeTransferBuffer = nullptr;
    SDL_GPUBuffer* Renderer2D::SpriteComputeBuffer = nullptr;
    SDL_GPUBuffer* Renderer2D::SpriteVertexBuffer = nullptr;
    SDL_GPUBuffer* Renderer2D::SpriteIndexBuffer = nullptr;
    SDL_GPUBuffer* Renderer2D::EffectVertexBuffer = nullptr;
    SDL_GPUBuffer* Renderer2D::EffectIndexBuffer = nullptr;
    SDL_GPUTexture* Renderer2D::SceneColorTexture;
    SDL_GPUTexture* Renderer2D::imguiTexture;

    SDL_GPUTexture* Renderer2D::swapchainTexture = nullptr;
    SDL_GPUCommandBuffer* Renderer2D::cmdBuf = nullptr;
    SDL_GPUColorTargetInfo Renderer2D::color_target_info = {};
    SDL_FColor Renderer2D::color = {0.0f, 0.0f, 0.0f, 1.0f};
    SDL_GPUTexture* Renderer2D::depthStencilTexture = nullptr;
    SDL_GPUTexture* Renderer2D::pixelEntityID = nullptr;
    int Renderer2D::SceneWidth, Renderer2D::SceneHeight;
    SDL_GPUViewport Renderer2D::viewport = {};
    SDL_GPUTransferBuffer* Renderer2D::downloadTransferBuffer = nullptr;
    Uint32* Renderer2D::id_data = nullptr;
    bool Renderer2D::EditorMode = false;

    /*struct QuadInstance
    {
        glm::vec3 Position;
        float Rotation;//check if compute shader uses radians or not glm::radians(rotation)
        float Scale;
        glm::vec2 size;
        glm::vec4 Color;
        //glm::vec2 TexCoord;
        //TODO: texid
        float textureIndex;
    } QuadInstance;#1#
    typedef struct QuadInstance
    {
        //float x, y, z;
        glm::vec3 position;
        float _pad0;
        glm::vec3 rotation;
        float _pad1;
        //float w, h, padding_a, padding_b;
        glm::vec2 size;
        glm::vec2 _pad2;     // 8 bytes padding
        glm::vec3 scale;
        float _pad3;
        glm::vec4 color;
        //float r, g, b, a;
        glm::vec2 tilePosition;
        glm::vec2 tileSize;
        glm::vec2 tileMultiplier;
        glm::vec2 atlasSize;
        
        // Editor-only
        glm::vec3 _pad4;
        int EntityID;
    } QuadInstance;

    struct Renderer2DData
    {
        const uint32_t MaxQuads = 10000;
        const uint32_t MaxVertices = MaxQuads * 4;
        const uint32_t MaxIndices = MaxQuads * 6;
    };

    static Renderer2DData s_data;
    std::vector<QuadInstance> m_QuadInstanceBuffer; // Store multiple quads
    constexpr Uint32 SPRITE_COUNT = 100000;
    uint32_t QuadIndexCount = 0;
    std::unordered_map<std::string, SDL_GPUTexture*> Renderer2D::TextureMap;

    void Renderer2D::Init(Window* windows)
    {
        VK_PROFILE_FUNCTION();

        window = windows;
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
                                     true, nullptr);
        if (!device)
        {
            SDL_Log("GPUCreateDevice failed");
        }

        const char* driver = SDL_GetGPUDeviceDriver(device);
        VK_CORE_INFO("GPU Driver: {0}", driver);

        if (!SDL_ClaimWindowForGPUDevice(device, window->getWindow()))
        {
            SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        }

        //----------------------------------------------------------
        //a bit confusing which one is active
        SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
        if (SDL_WindowSupportsGPUPresentMode(
            device,
            window->getWindow(),
            SDL_GPU_PRESENTMODE_IMMEDIATE
        ))
        {
            presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        }
        else if (SDL_WindowSupportsGPUPresentMode(
            device,
            window->getWindow(),
            SDL_GPU_PRESENTMODE_MAILBOX
        ))
        {
            presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
        }

        SDL_SetGPUSwapchainParameters(
            device,
            window->getWindow(),
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            presentMode
        );

        {
            VK_PROFILE_SCOPE("Create Render- and ComputePipeline");

            //maybe store in array or something idk shader library cherno video
            //add line, circle maybe ?, maybe triangle ?
            //maybe add even instead of files read the bytecode maybe faster perofrmance ask chatgpt looooooooool
            RenderPipeline = Shader::createsRenderPipeline(device, window->getWindow(),
                                                           "PositionColorTransform.vert", 0, 1, 0, 0,
                                                           "SolidColorDepth.frag", 2, 1, 0, 0);

            EffectPipeline = Shader::createsEffectPipeline(device, window->getWindow(),
                                                           "TexturedQuad.vert", 0, 0, 0, 0,
                                                           "DepthOutline.frag", 2, 1, 0, 0);

            // Create the sprite batch compute pipeline

            SDL_GPUComputePipelineCreateInfo compute_pipeline_create_info = {
                .num_readonly_storage_buffers = 1,
                .num_readwrite_storage_buffers = 1,
                .threadcount_x = 64,
                .threadcount_y = 1,
                .threadcount_z = 1
            };

            ComputePipeline = Shader::CreateComputePipelineFromShader(device, "shader.comp",
                                                                      &compute_pipeline_create_info);
        }

        {
            VK_PROFILE_SCOPE("Create White, Scene, Depth Texture");

            const SDL_PixelFormatDetails* pixelFormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
            SDL_Surface* whiteSurface = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA32);
            SDL_FillSurfaceRect(whiteSurface, nullptr, SDL_MapRGBA(pixelFormatDetails, nullptr, 255, 255, 255, 255));


            //Textures = VanK::Texture::CreateTexture("ChernoLogo.png", 4);
            //Textures = VanK::Texture::CreateTexture(whiteSurface);

            TextureMap["whiteSurface"] = VanK::Texture::CreateTexture2D(whiteSurface);

            // Create the Scene Textures
            {
                // Make them smaller so pixels stand out more
                int w, h;
                SDL_GetWindowSizeInPixels(window->getWindow(), &w, &h);
                SceneWidth = w;
                SceneHeight = h;

                SDL_GPUTextureCreateInfo texture_create_info{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = static_cast<Uint32>(SceneWidth),
                    .height = static_cast<Uint32>(SceneHeight),
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                SceneColorTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info
                );

                SDL_GPUTextureCreateInfo texture_create_info2{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                    .width = (Uint32)SceneWidth,
                    .height = (Uint32)SceneHeight,
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                depthStencilTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info2
                );

                SDL_GPUTextureCreateInfo texture_create_info3{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = static_cast<Uint32>(SceneWidth),
                    .height = static_cast<Uint32>(SceneHeight),
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                imguiTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info3
                );

                SDL_GPUTextureCreateInfo texture_create_info4{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R32_INT,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = static_cast<Uint32>(SceneWidth),
                    .height = static_cast<Uint32>(SceneHeight),
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                pixelEntityID = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info4
                );

                SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                    .size = static_cast<Uint32>(SceneWidth * SceneHeight * 4)
                };

                downloadTransferBuffer = SDL_CreateGPUTransferBuffer(
                device,
                &transfer_buffer_create_info
            );
            }
        }

        {
            VK_PROFILE_SCOPE("Create Sampler");

            SDL_GPUSamplerCreateInfo sampler_create_info = {
                .min_filter = SDL_GPU_FILTER_NEAREST,
                .mag_filter = SDL_GPU_FILTER_NEAREST,
                .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
                .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT, //clamp maybe
                .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT, //clamp maybe
                .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT //clamp maybe
            };

            Sampler = SDL_CreateGPUSampler(device, &sampler_create_info);
        }

        {
            VK_PROFILE_SCOPE("Create SpriteComputeTransferBuffer");

            SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info2 = {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = SPRITE_COUNT * sizeof(QuadInstance)
            };

            SpriteComputeTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info2);
        }

        {
            VK_PROFILE_SCOPE("Create SpriteComputeBuffer");

            SDL_GPUBufferCreateInfo buffer_create_info = {
                .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
                .size = SPRITE_COUNT * sizeof(QuadInstance)
            };

            SpriteComputeBuffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        }

        {
            VK_PROFILE_SCOPE("Create SpriteVertexBuffer");

            SDL_GPUBufferCreateInfo buffer_create_info2{
                .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = SPRITE_COUNT * 4 * sizeof(PositionTextureColorVertex)
            };

            SpriteVertexBuffer = SDL_CreateGPUBuffer(device, &buffer_create_info2);
        }
        {
            VK_PROFILE_SCOPE("Create SpriteIndexBuffer");

            SDL_GPUBufferCreateInfo buffer_create_info3 = {
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = SPRITE_COUNT * 6 * sizeof(Uint32)
            };

            SpriteIndexBuffer = SDL_CreateGPUBuffer(device, &buffer_create_info3);
        }

        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info3 = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = SPRITE_COUNT * 6 * sizeof(Uint32)
        };

        // Transfer the up-front data
        SDL_GPUTransferBuffer* indexBufferTransferBuffer = SDL_CreateGPUTransferBuffer(
            device, &transfer_buffer_create_info3);

        {
            VK_PROFILE_SCOPE("GPU UpFront indexBufferTransferBuffer");


            Uint32* indexTransferPtr = static_cast<Uint32*>(SDL_MapGPUTransferBuffer(
                device,
                indexBufferTransferBuffer,
                false
            ));

            for (Uint32 i = 0, j = 0; i < SPRITE_COUNT * 6; i += 6, j += 4)
            {
                indexTransferPtr[i + 0] = j + 0;
                indexTransferPtr[i + 1] = j + 1;
                indexTransferPtr[i + 2] = j + 2;
                indexTransferPtr[i + 3] = j + 0;
                indexTransferPtr[i + 4] = j + 2;
                indexTransferPtr[i + 5] = j + 3;
            }

            SDL_UnmapGPUTransferBuffer(
                device,
                indexBufferTransferBuffer
            );
        }

        {
            VK_PROFILE_SCOPE("Upload IndexBuffer");

            SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

            SDL_GPUTransferBufferLocation transfer_buffer_location{
                .transfer_buffer = indexBufferTransferBuffer,
                .offset = 0
            };

            SDL_GPUBufferRegion buffer_region = {
                .buffer = SpriteIndexBuffer,
                .offset = 0,
                .size = SPRITE_COUNT * 6 * sizeof(Uint32)
            };

            SDL_UploadToGPUBuffer(
                copyPass,
                &transfer_buffer_location,
                &buffer_region,
                false
            );

            /*SDL_DestroySurface(whiteSurface);#1#
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
            /*SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);#1#
            SDL_ReleaseGPUTransferBuffer(device, indexBufferTransferBuffer);
        }
        // Create & Upload Outline Effect Vertex and Index buffers
        {
            {
                VK_PROFILE_SCOPE("Create EffectVertexBuffer");

                SDL_GPUBufferCreateInfo buffer_create_info{
                    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                    .size = sizeof(PositionTextureVertex) * 4
                };

                EffectVertexBuffer = SDL_CreateGPUBuffer(
                    device,
                    &buffer_create_info
                );
            }

            {
                VK_PROFILE_SCOPE("Create EffectIndexBuffer");

                SDL_GPUBufferCreateInfo buffer_create_info2{
                    .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                    .size = sizeof(Uint16) * 6
                };

                EffectIndexBuffer = SDL_CreateGPUBuffer(
                    device,
                    &buffer_create_info2
                );
            }

            SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6)
            };

            SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
                device,
                &transfer_buffer_create_info
            );
            {
                VK_PROFILE_SCOPE("GPU UpFront transferData");

                PositionTextureVertex* transferData = static_cast<PositionTextureVertex*>(SDL_MapGPUTransferBuffer(
                    device,
                    bufferTransferBuffer,
                    false
                ));
                //maybe depth has to be 0,5 - 0,5 like in spritecomp ?
                transferData[0] = PositionTextureVertex{-1, 1, 0, 0, 0};
                transferData[1] = PositionTextureVertex{1, 1, 0, 1, 0};
                transferData[2] = PositionTextureVertex{1, -1, 0, 1, 1};
                transferData[3] = PositionTextureVertex{-1, -1, 0, 0, 1};

                Uint16* indexData = (Uint16*)&transferData[4];
                indexData[0] = 0;
                indexData[1] = 1;
                indexData[2] = 2;
                indexData[3] = 0;
                indexData[4] = 2;
                indexData[5] = 3;

                SDL_UnmapGPUTransferBuffer(device, bufferTransferBuffer);
            }

            {
                VK_PROFILE_SCOPE("Upload Effect Vertex and Index Buffer");

                SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
                SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

                SDL_GPUTransferBufferLocation transfer_buffer_location{
                    .transfer_buffer = bufferTransferBuffer,
                    .offset = 0
                };

                SDL_GPUBufferRegion buffer_region{
                    .buffer = EffectVertexBuffer,
                    .offset = 0,
                    .size = sizeof(PositionTextureVertex) * 4
                };

                SDL_UploadToGPUBuffer(
                    copyPass,
                    &transfer_buffer_location,
                    &buffer_region,
                    false
                );

                SDL_GPUTransferBufferLocation transfer_buffer_location2{
                    .transfer_buffer = bufferTransferBuffer,
                    .offset = sizeof(PositionTextureVertex) * 4
                };

                SDL_GPUBufferRegion buffer_region3{
                    .buffer = EffectIndexBuffer,
                    .offset = 0,
                    .size = sizeof(Uint16) * 6
                };

                SDL_UploadToGPUBuffer(
                    copyPass,
                    &transfer_buffer_location2,
                    &buffer_region3,
                    false
                );


                SDL_EndGPUCopyPass(copyPass);
                SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
                SDL_ReleaseGPUTransferBuffer(device, bufferTransferBuffer);
            }
        }
    }

    void Renderer2D::Shutdown()
    {
        VK_PROFILE_FUNCTION();
        SDL_ReleaseGPUTransferBuffer(device, downloadTransferBuffer);
        SDL_ReleaseGPUTexture(device, pixelEntityID);
        SDL_ReleaseGPUTexture(device, imguiTexture);
        SDL_ReleaseGPUTexture(device, depthStencilTexture);
        SDL_ReleaseGPUTexture(device, SceneColorTexture);
        VK_CORE_INFO("Shutdown Renderer");
        SDL_ReleaseGPUComputePipeline(device, ComputePipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, RenderPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, EffectPipeline);
        SDL_ReleaseGPUSampler(device, Sampler);
        for (auto& texture : TextureMap)
        {
            // because right now im overt
                SDL_ReleaseGPUTexture(device, texture.second);  
        }
        TextureMap.clear();
        SDL_ReleaseGPUTransferBuffer(device, SpriteComputeTransferBuffer);
        SDL_ReleaseGPUBuffer(device, SpriteComputeBuffer);
        SDL_ReleaseGPUBuffer(device, SpriteVertexBuffer);
        SDL_ReleaseGPUBuffer(device, SpriteIndexBuffer);
        SDL_ReleaseGPUBuffer(device, EffectVertexBuffer);
        SDL_ReleaseGPUBuffer(device, EffectIndexBuffer);

        SDL_ReleaseWindowFromGPUDevice(device, window->getWindow());
        SDL_DestroyGPUDevice(device);
    }

    void Renderer2D::CoreFrameBufferTextureResize(uint32_t width, uint32_t height)
    {
        // save width and height so we can check in editorlayer onupdate if size to viewport changed downloaddata uses this to check endsubmit width and height

        SceneWidth = width;
        SceneHeight = height;
        
        SDL_ReleaseGPUTransferBuffer(device, downloadTransferBuffer);
        SDL_ReleaseGPUTexture(device, pixelEntityID);
        SDL_ReleaseGPUTexture(device, imguiTexture);
        SDL_ReleaseGPUTexture(device, depthStencilTexture);
        SDL_ReleaseGPUTexture(device, SceneColorTexture);

        // Create the Scene Textures / Resize
            {
                SDL_GPUTextureCreateInfo texture_create_info{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = width,
                    .height = height,
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                SceneColorTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info
                );

                SDL_GPUTextureCreateInfo texture_create_info2{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                    .width = width,
                    .height = height,
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                depthStencilTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info2
                );

                SDL_GPUTextureCreateInfo texture_create_info3{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = width,
                    .height = height,
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                imguiTexture = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info3
                );

                SDL_GPUTextureCreateInfo texture_create_info4{
                    .type = SDL_GPU_TEXTURETYPE_2D,
                    .format = SDL_GPU_TEXTUREFORMAT_R32_INT,
                    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                    .width = width,
                    .height = height,
                    .layer_count_or_depth = 1,
                    .num_levels = 1,
                    .sample_count = SDL_GPU_SAMPLECOUNT_1
                };

                pixelEntityID = SDL_CreateGPUTexture(
                    device,
                    &texture_create_info4
                );

                SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                    .size = width * height * 4
                };

                downloadTransferBuffer = SDL_CreateGPUTransferBuffer(
                device,
                &transfer_buffer_create_info
            );
            }
    }

    void Renderer2D::BeginSubmit()
    {
        VK_PROFILE_FUNCTION();
        cmdBuf = SDL_AcquireGPUCommandBuffer(device);
        if (cmdBuf == nullptr)
        {
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        }

        /*if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window->getWindow(), &swapchainTexture, nullptr, nullptr))
        {
            SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        }#1#
    }

    void Renderer2D::BeginScene(const Camera& camera, float nearPlane, float farPlane, const glm::mat4& transform)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);
        
        SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(viewProj), sizeof(glm::mat4));

        float uniformData[2] = {nearPlane, farPlane};
        SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);
    }

    void Renderer2D::BeginScene(const EditorCamera& camera, float nearPlane, float farPlane)
    {
        VK_PROFILE_FUNCTION();

        glm::mat4 viewProj = camera.GetViewProjection();
        
        SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(viewProj), sizeof(glm::mat4));

        float uniformData[2] = {nearPlane, farPlane};
        SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);
    }

    // Global declarations
    int index = 0;

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        VK_PROFILE_FUNCTION();
        glm::mat4 cam = camera.GetViewProjectionMatrix();
        SDL_PushGPUVertexUniformData(cmdBuf, 0, glm::value_ptr(cam), sizeof(glm::mat4));

        float uniformData[2] = {camera.GetNearPlane(), camera.GetFarPlane()};
        SDL_PushGPUFragmentUniformData(cmdBuf, 0, uniformData, 8);
    }

    void Renderer2D::FlushBatch()
    {
        VK_PROFILE_FUNCTION();
        /*if (swapchainTexture != nullptr)
        {#1#
            // Build sprite instance transfer
            QuadInstance* dataPtr = static_cast<QuadInstance*>(SDL_MapGPUTransferBuffer(
                device,
                SpriteComputeTransferBuffer,
                true
            ));

            for (Uint32 i = 0; i < m_QuadInstanceBuffer.size(); i += 1)
            {
                dataPtr[i].position = m_QuadInstanceBuffer[i].position;
                dataPtr[i].rotation = m_QuadInstanceBuffer[i].rotation;
                dataPtr[i].size = m_QuadInstanceBuffer[i].size;
                dataPtr[i].scale = m_QuadInstanceBuffer[i].scale;
                dataPtr[i].color = m_QuadInstanceBuffer[i].color;
                dataPtr[i].tilePosition = m_QuadInstanceBuffer[i].tilePosition;
                dataPtr[i].tileSize = m_QuadInstanceBuffer[i].tileSize;
                dataPtr[i].tileMultiplier = m_QuadInstanceBuffer[i].tileMultiplier;
                dataPtr[i].atlasSize = m_QuadInstanceBuffer[i].atlasSize;
                dataPtr[i].EntityID = m_QuadInstanceBuffer[i].EntityID;
            }

            //bind texture in drawquad and if it reaches max it flushes everything or pops the first one ?
            // texture slot is max ? upload has a cycle attribute how would that work

            SDL_UnmapGPUTransferBuffer(device, SpriteComputeTransferBuffer);

            SDL_GPUTransferBufferLocation transfer_buffer_location{
                .transfer_buffer = SpriteComputeTransferBuffer,
                .offset = 0
            };

            SDL_GPUBufferRegion SDL_GPUBufferRegion{
                .buffer = SpriteComputeBuffer,
                .offset = 0,
                .size = SPRITE_COUNT * sizeof(QuadInstance)
            };

            // Upload instance data
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);
            SDL_UploadToGPUBuffer(
                copyPass,
                &transfer_buffer_location,
                &SDL_GPUBufferRegion,
                true
            );
            SDL_EndGPUCopyPass(copyPass);

            SDL_GPUStorageBufferReadWriteBinding sdl_gpu_storage_buffer_read_write_binding{
                .buffer = SpriteVertexBuffer,
                .cycle = true
            };

            // Set up compute pass to build vertex buffer
            SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
                cmdBuf,
                nullptr,
                0,
                &sdl_gpu_storage_buffer_read_write_binding,
                1
            );

            SDL_BindGPUComputePipeline(computePass, ComputePipeline);

            SDL_GPUBuffer* gpu_buffer{
                SpriteComputeBuffer,
            };

            SDL_BindGPUComputeStorageBuffers(
                computePass,
                0,
                &gpu_buffer,
                1
            );

            SDL_DispatchGPUCompute(computePass, m_QuadInstanceBuffer.size(), 1, 1);

            SDL_EndGPUComputePass(computePass);

            // Render sprites
            // Suppose you want two color targets.
            SDL_GPUColorTargetInfo color_target_infos[2] = {};
            
            /*color_target_info = {nullptr};
            color_target_info.texture = SceneColorTexture;
            //color_target_info.clear_color = getTargetInfoClearColor();
            color_target_info.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 0.0f};
            //color_target_info.clear_color = SDL_FColor{1.0f, 0.0f, 1.0f,1.0f};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;#1#

            color_target_infos[0].texture = SceneColorTexture;
            color_target_infos[0].clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 0.0f};
            color_target_infos[0].load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_infos[0].store_op = SDL_GPU_STOREOP_STORE;
            
            int clearValue = -1;  // This is -1, which is 0xFFFFFFFF in 32-bit integer
            color_target_infos[1].texture = pixelEntityID;
            color_target_infos[1].clear_color = *reinterpret_cast<SDL_FColor*>(&clearValue);
            color_target_infos[1].load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_infos[1].store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {nullptr};
            depth_stencil_target_info.texture = depthStencilTexture;
            depth_stencil_target_info.cycle = true;
            depth_stencil_target_info.clear_depth = 1;
            depth_stencil_target_info.clear_stencil = 0;
            depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
            depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
            depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
                cmdBuf,
                color_target_infos,
                2,
                &depth_stencil_target_info
            );

            SDL_BindGPUGraphicsPipeline(renderPass, RenderPipeline);

            SDL_GPUBufferBinding buffer_binding{
                .buffer = SpriteVertexBuffer
            };

            SDL_BindGPUVertexBuffers(
                renderPass,
                0,
                &buffer_binding,
                1
            );

            SDL_GPUBufferBinding buffer_binding2{
                .buffer = SpriteIndexBuffer
            };

            SDL_BindGPUIndexBuffer(
                renderPass,
                &buffer_binding2,
                SDL_GPU_INDEXELEMENTSIZE_32BIT
            );

            SDL_GPUTextureSamplerBinding texture_sampler_binding[2];

            // Initialize the first sampler
            texture_sampler_binding[0].texture = TextureMap["whiteSurface"];
            texture_sampler_binding[0].sampler = Sampler;
            // Initialize the second sampler
            texture_sampler_binding[1].texture = TextureMap["ChernoLogo.png"];
            texture_sampler_binding[1].sampler = Sampler;

            /*SDL_GPUTextureSamplerBinding texture_sampler_binding = {
                .texture = TextureMap["ChernoLogo.png"],
                .sampler = Sampler
            };#1#

            SDL_BindGPUFragmentSamplers(
                renderPass,
                0,
                texture_sampler_binding,
                2
            );

            SDL_DrawGPUIndexedPrimitives(
                renderPass,
                m_QuadInstanceBuffer.size() * 6,
                1,
                0,
                0,
                0
            );
            
            SDL_EndGPURenderPass(renderPass);

            // Render the Outline Effect that samples from the Color/Depth textures
            SDL_GPUColorTargetInfo swapchainTargetInfo = {nullptr};
            swapchainTargetInfo.texture = imguiTexture;
            swapchainTargetInfo.clear_color = getTargetInfoClearColor();//maybe dont need
            swapchainTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            swapchainTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            renderPass = SDL_BeginGPURenderPass(cmdBuf, &swapchainTargetInfo, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(renderPass, EffectPipeline);
            SDL_GPUBufferBinding gpu_buffer_binding3{.buffer = EffectVertexBuffer, .offset = 0};
            SDL_BindGPUVertexBuffers(renderPass, 0, &gpu_buffer_binding3, 1);
            SDL_GPUBufferBinding gpu_buffer_binding4{.buffer = EffectIndexBuffer, .offset = 0};
            SDL_BindGPUIndexBuffer(renderPass, &gpu_buffer_binding4, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            SDL_GPUTextureSamplerBinding samplers[2];

            // Initialize the first sampler
            samplers[0].texture = SceneColorTexture;
            samplers[0].sampler = Sampler;
            // Initialize the second sampler
            samplers[1].texture = depthStencilTexture;
            samplers[1].sampler = Sampler;

            if (viewport.w > 0)
            {
                SDL_SetGPUViewport(renderPass, &viewport);
            }
            
            SDL_BindGPUFragmentSamplers(renderPass, 0, samplers, 2);
            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
            SDL_EndGPURenderPass(renderPass);
        /*}#1#
        //m_QuadInstanceBuffer.clear();
        Stats.DrawCalls++;
    }

    void Renderer2D::EndScene()
    {
        FlushBatch();
    }

    void Renderer2D::EndSubmit()
    {
        VK_PROFILE_FUNCTION();

        /*SDL_GPUBlitInfo blit_info = {
            { swapchainTexture, static_cast<Uint32>(SceneWidth), static_cast<Uint32>(SceneHeight) },
            { SceneColorTexture, static_cast<Uint32>(SceneWidth), static_cast<Uint32>(SceneHeight) },
            SDL_GPU_LOADOP_DONT_CARE,
            SDL_GPU_FILTER_NEAREST
        };
        #1#
        VK_CORE_ASSERT("Editor Mode EndSubmint CopyPass Width and height bad {0}, {1}", SceneWidth, SceneHeight);
        if (!EditorMode)
        {
            SDL_SubmitGPUCommandBuffer(cmdBuf);
        } else
        {
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);
            SDL_GPUTextureRegion texture_region = {
                .texture = pixelEntityID,
                .w = static_cast<Uint32>(SceneWidth),
                .h = static_cast<Uint32>(SceneHeight),
                .d = 1
            };

            SDL_GPUTextureTransferInfo transfer_info = {
                .transfer_buffer = downloadTransferBuffer,
                .offset = 0
            };
        
            SDL_DownloadFromGPUTexture(copyPass, &texture_region, &transfer_info);
            SDL_EndGPUCopyPass(copyPass);

            SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf);
            SDL_WaitForGPUFences(device, true, &fence, 1);
            SDL_ReleaseGPUFence(device, fence);
        
            Uint8 *downloadedData = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(
                device,
                downloadTransferBuffer,
                false
            ));
        
            Uint32 *idData = reinterpret_cast<Uint32*>(downloadedData);
            setDownloadedData(idData);
            SDL_UnmapGPUTransferBuffer(device, downloadTransferBuffer);   
        }
        
        m_QuadInstanceBuffer.clear();
    }

    void Renderer2D::ClearColor()
    {
        setTargetInfoClearColor(SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f});
    }

    void Renderer2D::SetColor(const glm::vec4& color)
    {
        setTargetInfoClearColor(SDL_FColor{color.r, color.g, color.b, color.a});
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize,
                              glm::vec2 tileMultiplier, glm::vec2 atlasSize)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, color, tilePosition, tileSize,
                 tileMultiplier, atlasSize);
    }

    //atlas
    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec4& color, glm::vec2 tilePosition, glm::vec2 tileSize,
                              glm::vec2 tileMultiplier, glm::vec2 atlasSize)
    {
        if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
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

        Stats.QuadCount++;
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
        if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
        {
            /*FlushBatch();
            m_QuadInstanceBuffer.clear();#1#
            VK_CORE_INFO("to many draws make spritecount bigger");
        }

        QuadInstance quad;
        quad.position = position;
        quad.rotation = rotation;
        quad.scale = scale;
        quad.size = size;
        quad.color = color;
        quad.tilePosition = {0.0f, 0.0f};
        quad.tileSize = {0.0f, 0.0f};
        quad.tileMultiplier = {0.0f, 0.0f};
        quad.atlasSize = {0.0f, 0.0f};
        quad.EntityID = entityID;

        m_QuadInstanceBuffer.emplace_back(quad);

        Stats.QuadCount++;
    }

    //image
    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, const ::std::string
                              & ImageName, float TilingFactor, const glm::vec4& color, int entityID)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, ImageName, TilingFactor, color, entityID);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& scale,
                              const glm::vec3& rotation, ::std::string
                              TextureName, float TilingFactor, const glm::vec4& color, int entityID)
    {
        if (m_QuadInstanceBuffer.size() >= SPRITE_COUNT)
        {
            /*FlushBatch();
            m_QuadInstanceBuffer.clear();#1#
            VK_CORE_INFO("to many draws make spritecount bigger");
        }

        QuadInstance quad;
        quad.position = position;
        quad.rotation = rotation;
        quad.scale = scale;
        quad.size = size;
        quad.color = color;
        quad.tilePosition = {0.0f, 0.0f};
        quad.tileSize = {0.0f, 0.0f};
        quad.tileMultiplier = {0.0f, 0.0f};
        quad.atlasSize = {0.0f, 0.0f};
        quad.EntityID = entityID;
        
        TextureMap["whiteSurface"] = TextureMap[TextureName];

        m_QuadInstanceBuffer.emplace_back(quad);

        Stats.QuadCount++;
    }

    void Renderer2D::DrawSprite(TransformComponent tc, SpriteRendererComponent src, int entityID)
    {
        if (src.TextureName != "")
        {
            DrawQuad(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.TextureName, src.TilingFactor, src.Color, entityID);
        } else
        {
            DrawQuad(tc.Position, tc.Size, tc.Scale, tc.Rotation, src.Color, entityID);
        }
    }
    
    void Renderer2D::ResetStats()
    {
        std::memset(&Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats()
    {
        return Stats;
    }
}*/