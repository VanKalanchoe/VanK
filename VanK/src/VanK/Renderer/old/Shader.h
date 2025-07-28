/*
#pragma once

#include "VanK/Core/core.h"

namespace VanK
{
    class Shader
    {
    public:
        static SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const char* shaderFilename,
        Uint32 samplerCount,
        Uint32 uniformBufferCount,
        Uint32 storageBufferCount,
        Uint32 storageTextureCount);
        static SDL_GPUComputePipeline* CreateComputePipelineFromShader(SDL_GPUDevice* device, const char* shaderFilename,
                                                                       const SDL_GPUComputePipelineCreateInfo* createInfo);
        static SDL_GPUGraphicsPipeline* createsRenderPipeline(SDL_GPUDevice* device, SDL_Window* window,
                                                             const char* vertShaderName,
                                                             Uint32 vertSamplerCount, Uint32 vertUniformBufferCount,
                                                             Uint32 vertStorageBufferCount, Uint32 vertStorageTextureCount,
                                                             const char* fragShaderName, Uint32 fragSamplerCount,
                                                             Uint32 fragUniformBufferCount, Uint32 fragStorageBufferCount,
                                                             Uint32 fragStorageTextureCount);
        static SDL_GPUGraphicsPipeline* createsEffectPipeline(SDL_GPUDevice* device, SDL_Window* window,
                                                       const char* vertShaderName,
                                                       Uint32 vertSamplerCount, Uint32 vertUniformBufferCount,
                                                       Uint32 vertStorageBufferCount, Uint32 vertStorageTextureCount,
                                                       const char* fragShaderName, Uint32 fragSamplerCount,
                                                       Uint32 fragUniformBufferCount, Uint32 fragStorageBufferCount,
                                                       Uint32 fragStorageTextureCount);
    };
}
*/
