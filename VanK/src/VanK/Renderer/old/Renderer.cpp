/*
#include "Renderer.h"

#include "VanK/Core/Log.h"

//set viewport maybe in the future resizing the cherno video 19mins
namespace VanK
{
    Window* Renderer::window = nullptr;
    SDL_GPUDevice* Renderer::gpuDevice = nullptr;
    SDL_GPUGraphicsPipeline* Renderer::RenderPipeline = nullptr;
    SDL_GPUBuffer* Renderer::VertexBuffer = nullptr;
    SDL_GPUBuffer* Renderer::IndexBuffer = nullptr;
    SDL_GPUSampler* Renderer::Sampler = nullptr;
    SDL_GPUCommandBuffer* Renderer::CommandBuffer = nullptr;
    SDL_GPUTexture* Renderer::SwapchainTexture = nullptr;
    SDL_GPURenderPass* Renderer::RenderPass = nullptr;
    SDL_FColor Renderer::color = {0.0f, 0.0f, 0.0f, 1.0f};
    SDL_GPUColorTargetInfo Renderer::ColorTargetInfo = {};
    
    static Renderer::Statistics Stats;
    
    typedef struct Quad
    {
        glm::mat4 transform;
        glm::vec4 color;
        std::string textureName;
    } Quad;

    std::vector<Quad> QuadBuffer; // Store multiple quads
    
    std::unordered_map<std::string, SDL_GPUTexture*> Renderer::TextureMap;

    struct Renderer2DData
    {
        const uint32_t MaxQuads = 100000;
        const uint32_t MaxVertices = MaxQuads * 4;
        const uint32_t MaxIndices = MaxQuads * 6;
    };

    static Renderer2DData s_data;

    void Renderer::Init(Window* windowcpy)
    {
        window = windowcpy;
        gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
                                      true, nullptr);
        if (!gpuDevice)
        {
            SDL_Log("GPUCreateDevice failed");
        }

        const char* driver = SDL_GetGPUDeviceDriver(gpuDevice);
        VK_CORE_INFO("GPU Driver: {0}", driver);

        if (!SDL_ClaimWindowForGPUDevice(gpuDevice, window->getWindow()))
        {
            SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        }

        //a bit confusing which one is active
        SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
        if (SDL_WindowSupportsGPUPresentMode(
            gpuDevice,
            window->getWindow(),
            SDL_GPU_PRESENTMODE_IMMEDIATE
        ))
        {
            presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        }
        else if (SDL_WindowSupportsGPUPresentMode(
            gpuDevice,
            window->getWindow(),
            SDL_GPU_PRESENTMODE_MAILBOX
        ))
        {
            presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
        }

        SDL_SetGPUSwapchainParameters(
            gpuDevice,
            window->getWindow(),
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            presentMode
        );

        //maybe store in array or something idk shader library cherno video
        //add line, circle maybe ?, maybe triangle ?
        //maybe add even instead of files read the bytecode maybe faster perofrmance ask chatgpt looooooooool
        RenderPipeline = Shader::createsRenderPipeline(gpuDevice, window->getWindow(),
                                          "shader.vert", 0, 2, 0, 0,
                                          "shader.frag", 1, 2, 0, 0);

        const SDL_PixelFormatDetails* pixelFormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
        SDL_Surface* whiteSurface = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA32);
        SDL_FillSurfaceRect(whiteSurface, nullptr, SDL_MapRGBA(pixelFormatDetails, nullptr, 255, 255, 255, 255));
        TextureMap["whiteSurface"] = VanK::Texture::CreateTexture(whiteSurface);
        
        //---------------------------------------------quad create
        
        // Create the vertex and index buffers
        SDL_GPUBufferCreateInfo buffer_create_infoVertex = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = static_cast<Uint32>(s_data.MaxVertices * sizeof(PositionTextureVertex))
        };

        VertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &buffer_create_infoVertex);

        SDL_GPUBufferCreateInfo buffer_create_infoIndex{
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = static_cast<Uint32>(s_data.MaxIndices * sizeof(Uint16))
        };

        IndexBuffer = SDL_CreateGPUBuffer(gpuDevice, &buffer_create_infoIndex);

        /#1#/ Call the texture creation function
        Texture = createTexture(nullptr);#1#

        // Create the sampler
        SDL_GPUSamplerCreateInfo sampler_create_info = {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        };

        Sampler = SDL_CreateGPUSampler(gpuDevice, &sampler_create_info);

        // Create and map the transfer buffer for vertex and index data
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = static_cast<Uint32>(s_data.MaxVertices * sizeof(PositionTextureVertex) + s_data.MaxIndices * sizeof(Uint16))
        };

        SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transfer_buffer_create_info);

        PositionTextureVertex* transferData = static_cast<PositionTextureVertex*>(SDL_MapGPUTransferBuffer(
            gpuDevice,
            bufferTransferBuffer,
            false
        ));

        
        transferData[0] = PositionTextureVertex{-0.5f, -0.5f, 0, 0, 0}; // bottom left
        transferData[1] = PositionTextureVertex{0.5f, -0.5f, 0, 1, 0}; // bottom right
        transferData[2] = PositionTextureVertex{0.5f, 0.5f, 0, 1, 1}; // top right
        transferData[3] = PositionTextureVertex{-0.5f, 0.5f, 0, 0, 1}; // top left

        Uint16* indexData = reinterpret_cast<Uint16*>(&transferData[4]);
        indexData[0] = 0;
        indexData[1] = 1;
        indexData[2] = 2;
        indexData[3] = 0;
        indexData[4] = 2;
        indexData[5] = 3;

        SDL_UnmapGPUTransferBuffer(gpuDevice, bufferTransferBuffer);

        // Upload the transfer data to the GPU resources
        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTransferBufferLocation transfer_buffer_location = {
            .transfer_buffer = bufferTransferBuffer,
            .offset = 0
        };

        SDL_GPUBufferRegion buffer_region = {
            .buffer = VertexBuffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex) * 4
        };

        SDL_UploadToGPUBuffer(copyPass, &transfer_buffer_location, &buffer_region, false);

        SDL_GPUTransferBufferLocation transfer_buffer_location2 = {
            .transfer_buffer = bufferTransferBuffer,
            .offset = sizeof(PositionTextureVertex) * 4
        };

        SDL_GPUBufferRegion buffer_region2 = {
            .buffer = IndexBuffer,
            .offset = 0,
            .size = sizeof(Uint16) * 6
        };

        SDL_UploadToGPUBuffer(copyPass, &transfer_buffer_location2, &buffer_region2, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
    }
    
    void Renderer::Shutdown()
    {
        VK_CORE_INFO("Renderer deleted");
        
        SDL_ReleaseGPUGraphicsPipeline(gpuDevice, RenderPipeline);
        
        for (const auto& resources : TextureMap)
        {
            SDL_ReleaseGPUTexture(gpuDevice, resources.second);  
        }
           
        SDL_ReleaseGPUSampler(gpuDevice, Sampler);
        SDL_ReleaseGPUBuffer(gpuDevice, VertexBuffer);
        SDL_ReleaseGPUBuffer(gpuDevice, IndexBuffer);
        SDL_ReleaseWindowFromGPUDevice(gpuDevice, window->getWindow());
        SDL_DestroyGPUDevice(gpuDevice);  
    }
        
    void Renderer::BeginSubmit()
    {
        CommandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
        if (CommandBuffer == nullptr)
        {
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        }

        if (!SDL_AcquireGPUSwapchainTexture(CommandBuffer, window->getWindow(), &SwapchainTexture, nullptr, nullptr))
        {
            SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        }
    }
    
    void Renderer::BeginScene(const OrthographicCamera& camera)
    {
        ColorTargetInfo = {nullptr};
        ColorTargetInfo.texture = SwapchainTexture;
        ColorTargetInfo.clear_color = GetTargetInfoClearColor();
        ColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        ColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        
        glm::mat4 cam = camera.GetViewProjectionMatrix();
        SDL_PushGPUVertexUniformData(CommandBuffer, 1, glm::value_ptr(cam), sizeof(glm::mat4));
    }
    
    void Renderer::FlushBatch()
    {
        if (SwapchainTexture != nullptr)
        {
            RenderPass = SDL_BeginGPURenderPass(CommandBuffer, &ColorTargetInfo, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(RenderPass, RenderPipeline);
        
            //bind buffers for drawing the quads
            SDL_GPUBufferBinding buffer_bindingVertex = {.buffer = VertexBuffer, .offset = 0};
            SDL_BindGPUVertexBuffers(RenderPass, 0, &buffer_bindingVertex, 1);
            SDL_GPUBufferBinding buffer_bindingIndex = {.buffer = IndexBuffer, .offset = 0};
            SDL_BindGPUIndexBuffer(RenderPass, &buffer_bindingIndex, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            // Now draw all the quads in the list
            for (size_t i = 0; i < QuadBuffer.size(); i++)
            {
                SDL_GPUTextureSamplerBinding texture_sampler_binding = {
                    .texture = TextureMap.contains(QuadBuffer[i].textureName) ? TextureMap[QuadBuffer[i].textureName] : TextureMap["whiteSurface"], .sampler = Sampler
                }; //textures are loaded but indexing wrong fix

                SDL_BindGPUFragmentSamplers(RenderPass, 0, &texture_sampler_binding, 1);

                glm::mat4 matrixUniforms = QuadBuffer[i].transform;
                glm::vec4 tintColor = QuadBuffer[i].color;

                // Push the matrix and fragment uniform data to the GPU
                SDL_PushGPUVertexUniformData(CommandBuffer, 0, glm::value_ptr(matrixUniforms), sizeof(glm::mat4));
                SDL_PushGPUFragmentUniformData(CommandBuffer, 0, glm::value_ptr(tintColor), sizeof(glm::vec4));

                SDL_DrawGPUIndexedPrimitives(RenderPass, 6, 1, 0, 0, 0);
            }
            SDL_EndGPURenderPass(RenderPass);
            Stats.DrawCalls++;
        } 
    }
    
    void Renderer::EndScene()
    {
        FlushBatch();
    }
    
    void Renderer::EndSubmit()
    {
        QuadBuffer.clear(); 
        SDL_SubmitGPUCommandBuffer(CommandBuffer);
    }
    
    void Renderer::ClearColor()
    {
        SetTargetInfoClearColor(SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f}); 
    }
    
    void Renderer::SetColor(const glm::vec4& color)
    {
        SetTargetInfoClearColor(SDL_FColor{color.r, color.g, color.b, color.a});
    }
    
    //Primitvies
    //Quad
    void Renderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const glm::vec4& color)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, color);
    }
    
    void Renderer::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const glm::vec4& color)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, "whiteSurface", 1.0f, color);
    }
    
    void Renderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const std::string ImageName, float TilingFactor, const glm::vec4& color)
    {
        DrawQuad(glm::vec3(position.x, position.y, 0.0f), size, scale, rotation, ImageName, TilingFactor, color); 
    }
    
    void Renderer::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec2& scale, const float& rotation, const std::string ImageName, float TilingFactor, const glm::vec4& color)
    {
        Quad quad;
        quad.transform = glm::translate(glm::mat4(1.0f), position) * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(scale.x * size.x, scale.y * size.y, 1.0f));
        quad.color = color;
        quad.textureName = ImageName;

        QuadBuffer.emplace_back(quad);
        
        SDL_PushGPUFragmentUniformData(CommandBuffer, 1, &TilingFactor, sizeof(float));
        
        Stats.QuadCount++;
    }

    void Renderer::ResetStats()
    {
        std::memset(&Stats, 0, sizeof(Statistics));
    }

    Renderer::Statistics Renderer::GetStats()
    {
        return Stats;
    }
}















/*
#include "Renderer.h"
//set viewport maybe in the future resizing the cherno video 19mins
namespace VanK
{
    OrthographicCamera* orthoCamera = new OrthographicCamera(-1.6f, 1.6f, -0.9f, 0.9f);
    
    struct GPUResources
    {
        SDL_GPUBuffer* vertexBuffer;
        SDL_GPUBuffer* indexBuffer;
        SDL_GPUTexture* texture;
        SDL_GPUSampler* sampler;
        SDL_GPUTransferBuffer* bufferTransferBuffer;
        SDL_GPUTransferBuffer* textureTransferBuffer;

        // Add a transformation matrix for each quad
        glm::mat4 transform; // Transformation matrix (position, scale, rotation)
        glm::vec4 color; // Color (if needed for fragment shader)
    };

    // A vector to store multiple sets of GPU resources
    std::vector<GPUResources> gpuResourcesList;

    Renderer::Renderer(Application& app, Window& window) : m_App(app), m_Window(window)
    {
        std::cout << "new Renderer Class" << std::endl;
        initRenderer();
    }

    Renderer::~Renderer()
    {
        std::cout << "Renderer deleted" << std::endl;
        SDL_ReleaseGPUGraphicsPipeline(device, Pipeline);
        for (const auto& resources : gpuResourcesList)
        {
            SDL_ReleaseGPUTexture(device, resources.texture);
            SDL_ReleaseGPUSampler(device, resources.sampler);
            SDL_ReleaseGPUBuffer(device, resources.vertexBuffer);
            SDL_ReleaseGPUBuffer(device, resources.indexBuffer);
        }
        SDL_ReleaseWindowFromGPUDevice(device, m_Window.getWindow());
        SDL_DestroyGPUDevice(device);
    }

    /*void Renderer::OnWindowResize(int width, int height)
    {
        float m_AspectRatio = width / height;
        cameras = orthoCamera->GetViewProjectionMatrix();
        orthoCamera->SetProjection(-m_AspectRatio, m_AspectRatio,-m_AspectRatio, m_AspectRatio);
    }#2#

    void Renderer::initRenderer()
    {
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
                                     true, nullptr);
        if (!device)
        {
            SDL_Log("GPUCreateDevice failed");
        }

        const char* driver = SDL_GetGPUDeviceDriver(device);
        SDL_Log("GPU driver: %s", driver);

        if (!SDL_ClaimWindowForGPUDevice(device, m_Window.getWindow()))
        {
            SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        }

        //maybe store in array or something idk shader library cherno video
        //add line, circle maybe ?, maybe triangle ?
        //maybe add even instead of files read the bytecode maybe faster perofrmance ask chatgpt looooooooool
        Pipeline = Shader::createPipeline(device, m_Window.getWindow(),
            "shader.vert", 0, 2, 0, 0,
            "shader.frag", 1, 1, 0, 0);
    }

    void Renderer::BeginSubmit()
    {
        cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        if (cmdbuf == nullptr)
        {
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        }
    }

    void Renderer::BeginScene(OrthographicCamera& camera)
    {
        if (!SDL_AcquireGPUSwapchainTexture(cmdbuf, m_Window.getWindow(), &swapchainTexture, nullptr, nullptr))
        {
            SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        }

        if (swapchainTexture != nullptr)
        {
            colorTargetInfo = {nullptr};
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.clear_color = getTargetInfoClearColor();
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(renderPass, Pipeline);
        }
        
        cameras = camera.GetViewProjectionMatrix();
        SDL_PushGPUVertexUniformData(cmdbuf, 1, glm::value_ptr(cameras), sizeof(glm::mat4));
    }

    void Renderer::EndScene()
    {
        RenderBatch();
        SDL_EndGPURenderPass(renderPass);
        //renderPass = nullptr;
    }

    void Renderer::EndSubmit()
    {
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        currentQuadIndex = 0; 
    }

    void Renderer::clearColor()
    {
        setTargetInfoClearColor(SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f});
    }

    void Renderer::setColor(float r, float g, float b, float a)
    {
        setTargetInfoClearColor(SDL_FColor{r, g, b, a});
    }

    void Renderer::Bind() const
    {
    }

    void Renderer::createQuad(SDL_Surface* imageData) const
    {
        if (imageData == nullptr)
        {
            //white texture as fallback if no image
            // Get the pixel format details for RGBA32 (replace SDL_PIXELFORMAT_RGBA32)
            const SDL_PixelFormatDetails* pixelFormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
            SDL_Surface* whiteSurface = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA32);
            SDL_FillSurfaceRect(whiteSurface, nullptr, SDL_MapRGBA(pixelFormatDetails, nullptr, 255, 255, 255, 255));
            imageData = whiteSurface;
        }


        SDL_GPUBuffer* VertexBuffer = nullptr;
        SDL_GPUBuffer* IndexBuffer = nullptr;
        SDL_GPUTexture* Texture = nullptr;
        SDL_GPUSampler* Sampler = nullptr;
        // Store the GPU resources
        GPUResources resources = {
            .vertexBuffer = VertexBuffer,
            .indexBuffer = IndexBuffer,
            .texture = Texture,
            .sampler = Sampler,
        };
//can do this once ------------------------
        SDL_GPUBufferCreateInfo buffer_create_infoVertex = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex) * 4
        };

        // Create the GPU resources
        VertexBuffer = SDL_CreateGPUBuffer(device, &buffer_create_infoVertex);
        resources.vertexBuffer = VertexBuffer;
        
        SDL_GPUBufferCreateInfo buffer_create_infoIndex{
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * 6
        };

        IndexBuffer = SDL_CreateGPUBuffer(device, &buffer_create_infoIndex);
        resources.indexBuffer = IndexBuffer;
    //----------------------------    
        SDL_GPUTextureCreateInfo texture_create_info = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = static_cast<unsigned>(imageData->w),
            .height = static_cast<unsigned>(imageData->h),
            .layer_count_or_depth = 1,
            .num_levels = 1
        };

        Texture = SDL_CreateGPUTexture(device, &texture_create_info);
        resources.texture = Texture;
        
        SDL_GPUSamplerCreateInfo sampler_create_info = {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        };

        Sampler = SDL_CreateGPUSampler(device, &sampler_create_info);
        resources.sampler = Sampler;
        
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6)
        };

        // Set up buffer data
        SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);

        PositionTextureVertex* transferData = static_cast<PositionTextureVertex*>(SDL_MapGPUTransferBuffer(
            device,
            bufferTransferBuffer,
            false
        ));

        transferData[0] = PositionTextureVertex{-0.5f, -0.5f, 0, 0, 0}; // bottom left
        transferData[1] = PositionTextureVertex{0.5f, -0.5f, 0, 1, 0}; // bottom right
        transferData[2] = PositionTextureVertex{0.5f, 0.5f, 0, 1, 1}; // top right
        transferData[3] = PositionTextureVertex{-0.5f, 0.5f, 0, 0, 1}; // top left

        Uint16* indexData = reinterpret_cast<Uint16*>(&transferData[4]);
        indexData[0] = 0;
        indexData[1] = 1;
        indexData[2] = 2;
        indexData[3] = 0;
        indexData[4] = 2;
        indexData[5] = 3;

        SDL_UnmapGPUTransferBuffer(device, bufferTransferBuffer);

        // Set up texture data
        const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4;
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info2 = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = imageSizeInBytes
        };

        SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
            device, &transfer_buffer_create_info2);

        Uint8* textureTransferPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(
            device,
            textureTransferBuffer,
            false
        ));

        SDL_memcpy(textureTransferPtr, imageData->pixels, imageSizeInBytes);
        SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

        // Upload the transfer data to the GPU resources
        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTransferBufferLocation transfer_buffer_location = {
            .transfer_buffer = bufferTransferBuffer,
            .offset = 0
        };

        SDL_GPUBufferRegion buffer_region = {
            .buffer = VertexBuffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex) * 4
        };

        SDL_UploadToGPUBuffer(copyPass, &transfer_buffer_location, &buffer_region, false);

        SDL_GPUTransferBufferLocation transfer_buffer_location2 = {
            .transfer_buffer = bufferTransferBuffer,
            .offset = sizeof(PositionTextureVertex) * 4
        };

        SDL_GPUBufferRegion buffer_region2 = {
            .buffer = IndexBuffer,
            .offset = 0,
            .size = sizeof(Uint16) * 6
        };

        SDL_UploadToGPUBuffer(copyPass, &transfer_buffer_location2, &buffer_region2, false);

        SDL_GPUTextureTransferInfo texture_transfer_info = {
            .transfer_buffer = textureTransferBuffer,
            .offset = 0,
        };

        SDL_GPUTextureRegion texture_region = {
            .texture = Texture,
            .w = static_cast<unsigned>(imageData->w),
            .h = static_cast<unsigned>(imageData->h),
            .d = 1
        };

        SDL_UploadToGPUTexture(copyPass, &texture_transfer_info, &texture_region, false);

        SDL_DestroySurface(imageData);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(device, bufferTransferBuffer);
        SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

        gpuResourcesList.emplace_back(resources);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, glm::vec3 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, glm::vec3 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 nullptr);
    }
    void Renderer::DrawQuad(glm::vec3 translationMatrix, glm::vec2 size, glm::vec3 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(size.x, size.y, 1.0f)), glm::vec4(tintColor, 1.0f),
                 nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, glm::vec4 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, glm::vec4 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, glm::vec3 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, glm::vec3 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, glm::vec4 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, glm::vec4 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, std::optional<float> scale, SDL_Surface* imageData)
    {
        float scaling = scale.has_value() ? scale.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scaling * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, std::optional<float> scale, SDL_Surface* imageData)
    {
        float scaling = scale.has_value() ? scale.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scaling * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, float scale, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, float scale, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float scale, glm::vec3 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float scale, glm::vec3 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float scale, glm::vec4 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float scale, glm::vec4 tintColor, SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, std::optional<float> rotation, glm::vec3 tintColor,
                            SDL_Surface* imageData)
    {
        float rotationAngle = rotation.has_value() ? rotation.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 1.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, std::optional<float> rotation, glm::vec3 tintColor,
                            SDL_Surface* imageData)
    {
        float rotationAngle = rotation.has_value() ? rotation.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f),
                 imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, std::optional<float> rotation, glm::vec4 tintColor,
                            SDL_Surface* imageData)
    {
        float rotationAngle = rotation.has_value() ? rotation.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, std::optional<float> rotation, glm::vec4 tintColor,
                            SDL_Surface* imageData)
    {
        float rotationAngle = rotation.has_value() ? rotation.value() : 0.0f;
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }


    void Renderer::DrawQuad(glm::vec2 translationMatrix)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)), glm::mat4(0.0f), glm::mat4(1.0f),
                 glm::vec4(1.0f), nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix), glm::mat4(0.0f), glm::mat4(1.0f), glm::vec4(1.0f),
                 nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)), glm::mat4(1.0f), glm::vec4(1.0f),
                 nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)), glm::mat4(1.0f), glm::vec4(1.0f),
                 nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, float scale)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, float scale)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(1.0f), nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, float scale, glm::vec3 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f), nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, float scale, glm::vec4 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, float scale, glm::vec3 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec4(tintColor, 1.0f), nullptr);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, float scale, glm::vec4 tintColor)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, nullptr);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, float rotation, float scale, glm::vec4 tintColor,
                            SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec2 translationMatrix, glm::vec2 size, float rotation, float scale, glm::vec4 tintColor,
                            SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(translationMatrix, 0.0f)),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(size.x, size.y, 1.0f)), tintColor, imageData);
    }

    void Renderer::DrawQuad(glm::vec3 translationMatrix, float rotation, float scale, glm::vec4 tintColor,
                            SDL_Surface* imageData)
    {
        DrawQuad(glm::translate(glm::mat4(1.0f), translationMatrix),
                 glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)),
                 glm::scale(glm::mat4(1.0f), scale * glm::vec3(1.0f, 1.0f, 1.0f)), tintColor, imageData);
    }

    // fix add radians everywehere
    void Renderer::DrawQuad(glm::mat4 translationMatrix, glm::mat4 rotationMatrix, glm::mat4 scale, glm::vec4 tintColor, SDL_Surface* imageData)
    {
        // Define the maximum number of quads per batch
        int maxBatchSize = 20000;
        
        if (currentQuadIndex == gpuResourcesList.size())
        {
            std::cout << "Creating a new quad resource! Current size: " << gpuResourcesList.size() << "\n";

            createQuad(imageData);

            gpuResourcesList[currentQuadIndex].transform = translationMatrix * rotationMatrix * scale;
            gpuResourcesList[currentQuadIndex].color = tintColor;
        }
        else
        {
            // If the quad already exists, just update its transformation and color
            gpuResourcesList[currentQuadIndex].transform = translationMatrix * rotationMatrix * scale;
            gpuResourcesList[currentQuadIndex].color = tintColor;
        }

        // Increment the index for the next quad
        currentQuadIndex++;
        if (currentQuadIndex >= maxBatchSize)
        {
            // Now draw the quad to the screen (this could be a function to render quads)
            RenderBatch();
        } 
    }

    void Renderer::RenderBatch()
    {
        if (swapchainTexture != nullptr)
        {//replace index and vertex with 1 buffer becuase they all quads no need for multiple
            SDL_GPUBufferBinding buffer_bindingVertex = {.buffer = gpuResourcesList[0].vertexBuffer, .offset = 0};
            SDL_BindGPUVertexBuffers(renderPass, 0, &buffer_bindingVertex, 1);
            SDL_GPUBufferBinding buffer_bindingIndex = {.buffer = gpuResourcesList[0].indexBuffer, .offset = 0};
            SDL_BindGPUIndexBuffer(renderPass, &buffer_bindingIndex, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            for (size_t i = 0; i < gpuResourcesList.size(); i++)
            {
                
                SDL_GPUTextureSamplerBinding texture_sampler_binding = {
                    .texture = gpuResourcesList[i].texture, .sampler = gpuResourcesList[0].sampler
                };
                SDL_BindGPUFragmentSamplers(renderPass, 0, &texture_sampler_binding, 1);
                
                glm::mat4 matrixUniforms = gpuResourcesList[i].transform;
                
                glm::vec4 tintColor = gpuResourcesList[i].color;
                
                // Push the matrix and fragment uniform data to the GPU
                SDL_PushGPUVertexUniformData(cmdbuf, 0, glm::value_ptr(matrixUniforms), sizeof(glm::mat4));
                
                SDL_PushGPUFragmentUniformData(cmdbuf, 0, glm::value_ptr(tintColor), sizeof(glm::vec4));

                SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
            }
        }
    }
}
#1#
*/
