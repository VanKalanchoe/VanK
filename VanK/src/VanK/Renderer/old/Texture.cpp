/*#include "Texture.h"

namespace VanK
{
    SDL_Surface* Texture::LoadImage(const char* imageFilename, int desiredChannels)
    {
        char fullPath[256];
        SDL_Surface* result;
        SDL_PixelFormat format;

        SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Images/%s", SDL_GetBasePath(), imageFilename);

        result = IMG_Load(fullPath);
        if (result == nullptr)
        {
            SDL_Log("Failed to load Image: %s", SDL_GetError());
            return nullptr;
        }
        SDL_FlipSurface(result, SDL_FLIP_VERTICAL);
        if (desiredChannels == 4)
        {
            format = SDL_PIXELFORMAT_ABGR8888;
        }
        else
        {
            SDL_assert(!"Unexpected desiredChannels");
            SDL_DestroySurface(result);
            return nullptr;
        }
        if (result->format != format)
        {
            SDL_Surface* next = SDL_ConvertSurface(result, format);
            SDL_DestroySurface(result);
            result = next;
        }

        return result;
    }
    
    void Texture::CreateTexture2D(const char* imageFilename, int desiredChannels)
    {
        SDL_Surface* imageData = LoadImage(imageFilename, desiredChannels);
        if (imageData == nullptr)
        {
            SDL_Log("Could not load image data!");
            // Create a white texture as a fallback if no image is passed
            /*const SDL_PixelFormatDetails* pixelFormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
            imageData = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA32);
            SDL_FillSurfaceRect(imageData, nullptr, SDL_MapRGBA(pixelFormatDetails, nullptr, 255, 255, 255, 255));#1#
        }

        const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4;
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = imageSizeInBytes
        };

        SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
            Renderer2D::getDevice(),
            &transfer_buffer_create_info
        );

        Uint8* textureTransferPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(
            Renderer2D::getDevice(),
            textureTransferBuffer,
            false
        ));

        SDL_memcpy(textureTransferPtr, imageData->pixels, imageSizeInBytes);
        SDL_UnmapGPUTransferBuffer(Renderer2D::getDevice(), textureTransferBuffer);

        SDL_GPUTextureCreateInfo texture_create_info = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            .width = static_cast<Uint32>(imageData->w),
            .height = static_cast<Uint32>(imageData->h),
            .layer_count_or_depth = 1,
            .num_levels = 1
        };

        // Create the GPU resources
        SDL_GPUTexture* Textures = SDL_CreateGPUTexture(Renderer2D::getDevice(), &texture_create_info);

        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(Renderer2D::getDevice());
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTextureTransferInfo texture_transfer_info{
            .transfer_buffer = textureTransferBuffer,
            .offset = 0, /* Zeroes out the rest #1#
        };

        SDL_GPUTextureRegion texture_region{
            .texture = Textures,
            .w = static_cast<Uint32>(imageData->w),
            .h = static_cast<Uint32>(imageData->h),
            .d = 1
        };

        SDL_UploadToGPUTexture(
            copyPass,
            &texture_transfer_info,
            &texture_region,
            true
        );

        SDL_DestroySurface(imageData);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(Renderer2D::getDevice(), textureTransferBuffer);

        Renderer2D::TextureMap[imageFilename] = Textures;
    }

    SDL_GPUTexture* Texture::CreateTexture2D(SDL_Surface* imageData)
    {
        // Create a white texture as a fallback if no image is passed
        const SDL_PixelFormatDetails* pixelFormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_ABGR8888);
        imageData = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_ABGR8888);
        SDL_FillSurfaceRect(imageData, nullptr, SDL_MapRGBA(pixelFormatDetails, nullptr, 255, 255, 255, 255));

        const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4;
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = imageSizeInBytes
        };

        SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
            Renderer2D::getDevice(),
            &transfer_buffer_create_info
        );

        Uint8* textureTransferPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(
            Renderer2D::getDevice(),
            textureTransferBuffer,
            false
        ));

        SDL_memcpy(textureTransferPtr, imageData->pixels, imageSizeInBytes);
        SDL_UnmapGPUTransferBuffer(Renderer2D::getDevice(), textureTransferBuffer);

        SDL_GPUTextureCreateInfo texture_create_info = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            .width = static_cast<Uint32>(imageData->w),
            .height = static_cast<Uint32>(imageData->h),
            .layer_count_or_depth = 1,
            .num_levels = 1
        };

        // Create the GPU resources
        SDL_GPUTexture* Textures = SDL_CreateGPUTexture(Renderer2D::getDevice(), &texture_create_info);

        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(Renderer2D::getDevice());
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTextureTransferInfo texture_transfer_info{
            .transfer_buffer = textureTransferBuffer,
            .offset = 0, /* Zeroes out the rest #1#
        };

        SDL_GPUTextureRegion texture_region{
            .texture = Textures,
            .w = static_cast<Uint32>(imageData->w),
            .h = static_cast<Uint32>(imageData->h),
            .d = 1
        };

        SDL_UploadToGPUTexture(
            copyPass,
            &texture_transfer_info,
            &texture_region,
            true
        );

        SDL_DestroySurface(imageData);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(Renderer2D::getDevice(), textureTransferBuffer);

        return Textures;
    }
}

/*
*Yes, you've got it right! Here's a breakdown of what's happening:

1: Texture Upload (Initialization Phase):

When you load your textures, you upload them to GPU memory once, typically during your initialization or asset loading phase. This is usually done by creating a texture object using something like SDL_CreateGPUTexture (or the relevant function for your library).
Important: The texture data is persistent once uploaded to the GPU, meaning you don’t need to upload it again unless you’re modifying the texture content (such as dynamic textures or render targets).

2: Texture Binding (Per Frame):

After the texture is uploaded to GPU memory, each frame you don’t need to upload it again. Instead, you just bind the texture to the relevant texture unit (using something like SDL_GPUTextureSamplerBinding).
Every frame: You can switch which textures are used by binding them to different texture samplers, but you don't need to re-upload the data to GPU memory. This can help save performance.
For example, you can have a texture sampler for each texture and bind the appropriate texture at the beginning of each frame to specify which one to use.

3:Texture Usage:

You’ll only need to upload textures again if you're changing their content dynamically (e.g., streaming new data to them or using them as render targets).
Once a texture is uploaded to the GPU, you can reuse it across multiple frames by simply binding it (without re-uploading the actual data).

To Summarize:
1: Upload once in the initialization phase for static textures.
2: Bind per frame to use them in rendering.
2: Only upload again if you need to modify the texture data (e.g., dynamic textures).
It’s a pretty efficient way of handling textures, and it makes the rendering process faster, since you don't have to repeatedly upload texture data to the GPU every frame.
#1#*/