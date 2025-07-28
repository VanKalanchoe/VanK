/*
#include "Shader.h"

#include "Renderer2D.h"

namespace VanK
{
    SDL_GPUShader* Shader::LoadShader(
        SDL_GPUDevice* device,
        const char* shaderFilename,
        Uint32 samplerCount,
        Uint32 uniformBufferCount,
        Uint32 storageBufferCount,
        Uint32 storageTextureCount)
    {
        // Auto-detect the shader stage from the file name for convenience
        SDL_GPUShaderStage stage;
        if (SDL_strstr(shaderFilename, ".vert"))
        {
            stage = SDL_GPU_SHADERSTAGE_VERTEX;
        }
        else if (SDL_strstr(shaderFilename, ".frag"))
        {
            stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }
        else
        {
            SDL_Log("Invalid shader stage!");
            return nullptr;
        }

        char fullPath[256];
        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        const char* entrypoint;

        if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)
        {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/SPIRV/%s.spv", SDL_GetBasePath(),
                         shaderFilename);
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entrypoint = "main";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL)
        {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/MSL/%s.msl", SDL_GetBasePath(),
                         shaderFilename);
            format = SDL_GPU_SHADERFORMAT_MSL;
            entrypoint = "main0";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
        {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/DXIL/%s.dxil", SDL_GetBasePath(),
                         shaderFilename);
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entrypoint = "main";
        }
        else
        {
            SDL_Log("%s", "Unrecognized backend shader format!");
            return nullptr;
        }

        size_t codeSize;
        void* code = SDL_LoadFile(fullPath, &codeSize);
        if (code == nullptr)
        {
            SDL_Log("Failed to load shader from disk! %s", fullPath);
            return nullptr;
        }

        SDL_GPUShaderCreateInfo shaderInfo = {
            .code_size = codeSize,
            .code = static_cast<Uint8*>(code),
            .entrypoint = entrypoint,
            .format = format,
            .stage = stage,
            .num_samplers = samplerCount,
            .num_storage_textures = storageTextureCount,
            .num_storage_buffers = storageBufferCount,
            .num_uniform_buffers = uniformBufferCount
        };

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
        if (shader == nullptr)
        {
            SDL_Log("Failed to create shader!");
            SDL_free(code);
            return nullptr;
        }

        SDL_free(code);
        return shader;
    }

    SDL_GPUComputePipeline* Shader::CreateComputePipelineFromShader(
    SDL_GPUDevice* device,
    const char* shaderFilename,
    const SDL_GPUComputePipelineCreateInfo *createInfo
) {
        char fullPath[256];
        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        const char *entrypoint;

        if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/SPIRV/%s.spv", SDL_GetBasePath(), shaderFilename);
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entrypoint = "main";
        } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/MSL/%s.msl", SDL_GetBasePath(), shaderFilename);
            format = SDL_GPU_SHADERFORMAT_MSL;
            entrypoint = "main0";
        } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
            SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/DXIL/%s.dxil", SDL_GetBasePath(), shaderFilename);
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entrypoint = "main";
        } else {
            SDL_Log("%s", "Unrecognized backend shader format!");
            return nullptr;
        }

        size_t codeSize;
        void* code = SDL_LoadFile(fullPath, &codeSize);
        if (code == nullptr)
        {
            SDL_Log("Failed to load compute shader from disk! %s", fullPath);
            return nullptr;
        }

        // Make a copy of the create data, then overwrite the parts we need
        SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
        newCreateInfo.code = static_cast<const Uint8*>(code);
        newCreateInfo.code_size = codeSize;
        newCreateInfo.entrypoint = entrypoint;
        newCreateInfo.format = format;

        SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
        if (pipeline == nullptr)
        {
            SDL_Log("Failed to create compute pipeline!");
            SDL_free(code);
            return nullptr;
        }

        SDL_free(code);
        return pipeline;
    }

    SDL_GPUGraphicsPipeline* Shader::createsRenderPipeline(SDL_GPUDevice* device, SDL_Window* window,
        const char* vertShaderName, Uint32 vertSamplerCount, Uint32 vertUniformBufferCount, Uint32 vertStorageBufferCount, Uint32 vertStorageTextureCount,
        const char* fragShaderName, Uint32 fragSamplerCount, Uint32 fragUniformBufferCount, Uint32 fragStorageBufferCount, Uint32 fragStorageTextureCount)
    {
        // Create the shaders
        SDL_GPUShader* vertShader = LoadShader(device, vertShaderName, vertSamplerCount, vertUniformBufferCount, vertStorageBufferCount, vertStorageTextureCount);
        if (vertShader == nullptr)
        {
            SDL_Log("Failed to create vertex shader!");
        }

        SDL_GPUShader* fragShader = LoadShader(device, fragShaderName, fragSamplerCount, fragUniformBufferCount, fragStorageBufferCount, fragStorageTextureCount);
        if (fragShader == nullptr)
        {
            SDL_Log("Failed to create fragment shader!");
        }

        // Define the color target descriptions separately
        SDL_GPUColorTargetDescription color_target_descriptions[] = {
            {
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .blend_state = {
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = true
                }
            },
            {//index texture
                .format = SDL_GPU_TEXTUREFORMAT_R32_INT,
                .blend_state = {
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = false
                }
            }
        };
        

        // Define the vertex buffer descriptions separately
        SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {
                .slot = 0,
                .pitch = sizeof(Renderer2D::PositionTextureColorVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0
            }
        };

        // Define the vertex attributes separately
        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = 0
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 16
            },
            {
                .location = 2,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = 32
            },
            {
                .location = 3,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_INT,
                .offset = 48
            }
        };

        SDL_GPUDepthStencilState depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .write_mask = 0xFF,
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false
        };

        SDL_GPURasterizerState rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        };
        //color attachment with texture
        SDL_GPUMultisampleState multisample_state = {
            .sample_count = SDL_GPU_SAMPLECOUNT_8,
            .sample_mask = 0xFFFFFFFF,
            .enable_mask = true
        };

        // Create the pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .vertex_shader = vertShader,
            .fragment_shader = fragShader,
            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffer_descriptions,
                .num_vertex_buffers = 1,
                .vertex_attributes = vertex_attributes,
                .num_vertex_attributes = 4
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state = rasterizer_state,
            .depth_stencil_state = depth_stencil_state,
            .target_info = {
                .color_target_descriptions = color_target_descriptions,
                .num_color_targets = 2,
                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                .has_depth_stencil_target = true
            },
        };

        SDL_GPUGraphicsPipeline* RenderPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

        if (RenderPipeline == nullptr)
        {
            SDL_Log("Failed to create pipeline!");
        }
    
        SDL_ReleaseGPUShader(device, vertShader);
        SDL_ReleaseGPUShader(device, fragShader);

        return RenderPipeline;
    }

    SDL_GPUGraphicsPipeline* Shader::createsEffectPipeline(SDL_GPUDevice* device, SDL_Window* window,
        const char* vertShaderName, Uint32 vertSamplerCount, Uint32 vertUniformBufferCount, Uint32 vertStorageBufferCount, Uint32 vertStorageTextureCount,
        const char* fragShaderName, Uint32 fragSamplerCount, Uint32 fragUniformBufferCount, Uint32 fragStorageBufferCount, Uint32 fragStorageTextureCount)
    {
        // Create the shaders
        SDL_GPUShader* vertShader = LoadShader(device, vertShaderName, vertSamplerCount, vertUniformBufferCount, vertStorageBufferCount, vertStorageTextureCount);
        if (vertShader == nullptr)
        {
            SDL_Log("Failed to create vertex shader!");
        }

        SDL_GPUShader* fragShader = LoadShader(device, fragShaderName, fragSamplerCount, fragUniformBufferCount, fragStorageBufferCount, fragStorageTextureCount);
        if (fragShader == nullptr)
        {
            SDL_Log("Failed to create fragment shader!");
        }

        // Define the color target descriptions separately
        SDL_GPUColorTargetDescription color_target_descriptions[] = {
            {
                //.format = SDL_GetGPUSwapchainTextureFormat(device, window),
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .blend_state = {
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = true
                }
            }
        };

        // Define the vertex buffer descriptions separately
        SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {
                .slot = 0,
                .pitch = sizeof(Renderer2D::PositionTextureVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0
            }
        };

        // Define the vertex attributes separately
        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = 0
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = sizeof(float) * 3
            }
        };
        

        // Create the pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .vertex_shader = vertShader,
            .fragment_shader = fragShader,
            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffer_descriptions,
                .num_vertex_buffers = 1,
                .vertex_attributes = vertex_attributes,
                .num_vertex_attributes = 2
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info = {
                .color_target_descriptions = color_target_descriptions,
                .num_color_targets = 1,
            },
        };

        SDL_GPUGraphicsPipeline* RenderPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

        if (RenderPipeline == nullptr)
        {
            SDL_Log("Failed to create pipeline!");
        }
    
        SDL_ReleaseGPUShader(device, vertShader);
        SDL_ReleaseGPUShader(device, fragShader);

        return RenderPipeline;
    }
}
*/
