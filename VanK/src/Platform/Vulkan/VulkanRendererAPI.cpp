#include "VulkanRendererAPI.h"
/*--
 * We are using the Vulkan Memory Allocator (VMA) to manage memory.
 * This is a library that helps to allocate memory for Vulkan resources.
-*/
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#define VMA_LEAK_LOG_FORMAT(format, ...)                                                                               \
{                                                                                                                    \
printf((format), __VA_ARGS__);                                                                                     \
printf("\n");                                                                                                      \
}
#include "vk_mem_alloc.h"

// To load JPG/PNG images
#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image/stb_image.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VanK/Renderer/RenderCommand.h"

namespace VanK
{
    //--- MinimalLatest ------------------------------------------------------------------------------------------------------------
    // Main class for the sample

    /*--
     * The application is the main class that is used to create the window, the Vulkan context, the swapchain, and the resources.
     *  - run the main loop.
     *  - render the scene.
    -*/
    
    VulkanRendererAPI* VulkanRendererAPI::s_instance = nullptr;

    VulkanRendererAPI::VulkanRendererAPI() = default;

    VulkanRendererAPI::VulkanRendererAPI(const Config& config) : m_window(config.window), m_windowSize({config.width, config.height})
    {
        // Set this instance as the static instance
        s_instance = this;

        // Vulkan Loader
        VK_CHECK(volkInitialize());
        
        init();
    }

    uint32_t VulkanRendererAPI::AddTextureToPool(utils::ImageResource&& imageResource)
    {
        // Add the texture to the image vector
        m_image.emplace_back(std::move(imageResource));

        // Update the descriptor set to include the new texture
        updateGraphicsDescriptorSet();

        // Return the index of the new texture
        return static_cast<uint32_t>(m_image.size() - 1);
    }

    void VulkanRendererAPI::RemoveTextureFromPool(uint32_t index)
    {
        // Safety checks
        if (index >= m_image.size())
        {
            LOGW("Attempted to remove texture at invalid index: %u (max: %zu)", index, m_image.size());
            return;
        }

        if (m_image.empty())
        {
            LOGW("Attempted to remove texture from empty pool");
            return;
        }

        // Destroy the texture resource
        m_allocator.destroyImageResource(m_image[index]);

        // Remove from vector
        m_image.erase(m_image.begin() + index);

        // Update descriptor set
        updateGraphicsDescriptorSet();

        LOGI("Removed texture at index %u, remaining textures: %zu", index, m_image.size());
    }

    void VulkanRendererAPI::ResizeDescriptor()
    {//this wont work anymore because i have multiple pipelines now fix it
        std::cout << "ResizeDescriptor: Recreating descriptor resources" << std::endl;

        VkDevice device = m_context.getDevice();
        VK_CHECK(vkDeviceWaitIdle(device));

        destroyGraphicsPipeline();

        vkDestroyDescriptorSetLayout(device, m_textureDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, m_commonDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

        m_textureDescriptorSetLayout = VK_NULL_HANDLE;
        m_commonDescriptorSetLayout = VK_NULL_HANDLE;
        m_descriptorPool = VK_NULL_HANDLE;
        m_textureDescriptorSet = VK_NULL_HANDLE;

        m_maxTextures += 100;

        createDescriptorPool();
        createGraphicDescriptorSet();
        createGraphicsPipeline(pipelineSpecifications);
    }

    VulkanRendererAPI& VulkanRendererAPI::Get()
    {
        if (!s_instance)
        {
            // If no instance is set, this will crash - which is what we want
            // because it means we're trying to use Vulkan before it's initialized
            throw std::runtime_error("VulkanRendererAPI not initialized! Call SetInstance first.");
        }
        return *s_instance;
    }

    VulkanRendererAPI::~VulkanRendererAPI()
    {
        // Clear the static instance if it's this instance
        if (s_instance == this)
        {
            s_instance = nullptr;
        }

        destroy();
    }

    /*--
     * Main initialization sequence
     * 
     * 1. Context Creation
     *    - Creates Vulkan instance with validation layers
     *    - Selects physical device (prefers discrete GPU)
     *    - Creates logical device with required extensions
     * 
     * 2. Resource Setup
     *    - Initializes VMA allocator for memory management
     *    - Creates command pools for graphics commands
     *    - Sets up swapchain for rendering
     * 
     * 3. Pipeline Creation
     *    - Creates descriptor layouts for textures and buffers
     *    - Sets up graphics pipeline with vertex/fragment shaders
     *    - Creates compute pipeline for vertex animation
     * 
     * 4. Resource Creation
     *    - Loads and uploads texture
     *    - Creates vertex and uniform buffers
     *    - Sets up ImGui for UI rendering
    -*/

    void VulkanRendererAPI::init()
    {
        // Create the Vulkan context
        m_context.init();

        // Initialize the VMA allocator
        m_allocator.init(VmaAllocatorCreateInfo{
            .physicalDevice = m_context.getPhysicalDevice(),
            .device = m_context.getDevice(),
            .instance = m_context.getInstance(),
            .vulkanApiVersion = m_context.getApiVersion(),
        });

        // Texture sampler pool
        m_samplerPool.init(m_context.getDevice());

        // Create the window surface
        //glfwCreateWindowSurface(m_context.getInstance(), m_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_surface));#
        SDL_Vulkan_CreateSurface(m_window, m_context.getInstance(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_surface));
        DBG_VK_NAME(m_surface);

        // Used for creating single-time command buffers
        createTransientCommandPool();

        // Create the swapchain
        m_swapchain.init(m_context.getPhysicalDevice(), m_context.getDevice(), m_context.getGraphicsQueue(), m_surface,
                         m_transientCmdPool);
        m_windowSize = m_swapchain.initResources(m_vSync); // Update the window size to the actual size of the surface

        // Create what is needed to submit the scene for each frame in-flight
        createFrameSubmission(m_swapchain.getMaxFramesInFlight());

        // Create a descriptor pool for creating descriptor set in the application
        createDescriptorPool();

        // Acquiring the sampler which will be used for displaying the GBuffer
        const VkSamplerCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .magFilter = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR
        };
        const VkSampler linearSampler = m_samplerPool.acquireSampler(info);
        DBG_VK_NAME(linearSampler);

        // Create the GBuffer, the target image and its depth buffer
        {
            VkCommandBuffer cmd = utils::beginSingleTimeCommands(m_context.getDevice(), m_transientCmdPool);

            const VkFormat depthFormat = utils::findDepthFormat(m_context.getPhysicalDevice());
            utils::GbufferCreateInfo gBufferInit{
                .device = m_context.getDevice(),
                .alloc = &m_allocator,
                .size = m_windowSize,
                .color = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32_SINT}, // Add second attachment for entity ID
                .depth = depthFormat,
                .linearSampler = linearSampler,
            };
            m_gBuffer.init(cmd, gBufferInit);

            utils::endSingleTimeCommands(cmd, m_context.getDevice(), m_transientCmdPool,
                                         m_context.getGraphicsQueue().queue);
        }

        // Create how resources are passed to the pipeline
        createGraphicDescriptorSet();
        

        /*// The image was loaded, now write its information, such that the graphic pipeline knows how to access it
        updateGraphicsDescriptorSet();*/
    }

    /*--
      * Destroy all resources and the Vulkan context
    -*/

    void VulkanRendererAPI::destroyGraphicsPipeline()
    {
        //maybe not destory all pipeplines only the one needed to be reloaded
        VkDevice device = m_context.getDevice();
        
        for (VkPipeline& pipeline : m_graphicsPipelines)
        {
            if (pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
        }
        for (VkPipelineLayout& pipelineLayout : m_graphicsPipelinesLayouts)
        {
            if (pipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
        }
    }

    void VulkanRendererAPI::destroyComputePipeline()
    {
        VkDevice device = m_context.getDevice();

        for (VkPipeline& pipeline : m_computePipelines)
        {
            if (pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
        }
        for (VkPipelineLayout& pipelineLayout : m_computePipelinesLayouts)
        {
            if (pipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
        }
    }

    void VulkanRendererAPI::waitForGraphicsQueueIdle()
    {
        vkQueueWaitIdle(m_context.getGraphicsQueue().queue);
    }

    void VulkanRendererAPI::destroy()
    {
        VkDevice device = m_context.getDevice();
        VK_CHECK(vkDeviceWaitIdle(device));
        m_allocator.freeStagingBuffers();
        m_swapchain.deinit();
        m_samplerPool.deinit();
        
        vkDestroyDescriptorPool(device, m_imguiDescriptorPool, nullptr); // Destroy ImGui pool

        vkFreeDescriptorSets(device, m_descriptorPool, 1, &m_textureDescriptorSet);
        
        destroyGraphicsPipeline();
        destroyComputePipeline();

        vkDestroyCommandPool(device, m_transientCmdPool, nullptr);
        vkDestroySurfaceKHR(m_context.getInstance(), m_surface, nullptr);
        vkDestroyDescriptorSetLayout(device, m_textureDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, m_commonDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

        // Frame info
        for (size_t i = 0; i < m_frameData.size(); i++)
        {
            vkFreeCommandBuffers(device, m_frameData[i].cmdPool, 1, &m_frameData[i].cmdBuffer);
            vkDestroyCommandPool(device, m_frameData[i].cmdPool, nullptr);
        }
        vkDestroySemaphore(device, m_frameTimelineSemaphore, nullptr);
        
        for (size_t i = 0; i < std::size(m_image); i++)
        {
            m_allocator.destroyImageResource(m_image[i]);
        }

        unmapDownloadedData();

        m_gBuffer.deinit();
        m_allocator.deinit();
        m_context.deinit();
    }

    /*--
     * Create a command pool for short lived operations
     * The command pool is used to allocate command buffers.
     * In the case of this sample, we only need one command buffer, for temporary execution.
    -*/

    void VulkanRendererAPI::createTransientCommandPool()
    {
        const VkCommandPoolCreateInfo commandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
            .queueFamilyIndex = m_context.getGraphicsQueue().familyIndex,
        };
        VK_CHECK(vkCreateCommandPool(m_context.getDevice(), &commandPoolCreateInfo, nullptr, &m_transientCmdPool));
        DBG_VK_NAME(m_transientCmdPool);
    }

    /*--
       * Creates a command pool (long life) and buffer for each frame in flight. Unlike the temporary command pool,
       * these pools persist between frames and don't use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT.
       * Each frame gets its own command buffer which records all rendering commands for that frame.
      -*/

    void VulkanRendererAPI::createFrameSubmission(uint32_t numFrames)
    {
        VkDevice device = m_context.getDevice();

        m_frameData.resize(numFrames);

        /*-- 
         * Initialize timeline semaphore with (numFrames - 1) to allow concurrent frame submission. See details in README.md
        -*/
        const uint64_t initialValue = (numFrames - 1);

        VkSemaphoreTypeCreateInfo timelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = initialValue,
        };

        /*-- 
         * Create timeline semaphore for GPU-CPU synchronization
         * This ensures resources aren't overwritten while still in use by the GPU
        -*/
        const VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timelineCreateInfo
        };
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_frameTimelineSemaphore));
        DBG_VK_NAME(m_frameTimelineSemaphore);

        /*-- 
         * Create command pools and buffers for each frame
         * Each frame gets its own command pool to allow parallel command recording while previous frames may still be executing on the GPU
        -*/
        const VkCommandPoolCreateInfo cmdPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = m_context.getGraphicsQueue().familyIndex,
        };

        for (uint32_t i = 0; i < numFrames; i++)
        {
            m_frameData[i].frameNumber = i; // Track frame index for synchronization

            // Separate pools allow independent reset/recording of commands while other frames are still in-flight
            VK_CHECK(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &m_frameData[i].cmdPool));
            DBG_VK_NAME(m_frameData[i].cmdPool);

            const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = m_frameData[i].cmdPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &m_frameData[i].cmdBuffer));
            DBG_VK_NAME(m_frameData[i].cmdBuffer);
        }
    }
    
    /*---
       * Begin frame is the first step in the rendering process.
       * It looks if the swapchain require rebuild, which happens when the window is resized.
       * It resets the command pool to reuse the command buffer for recording new rendering commands for the current frame.
       * It acquires the image from the swapchain to render into.
       * And it returns the command buffer for the frame.
    ---*/

    VkCommandBuffer VulkanRendererAPI::beginFrame()
    {
        VkDevice device = m_context.getDevice();

        if (m_swapchain.needRebuilding())
        {
            m_windowSize = m_swapchain.reinitResources(m_vSync);
        }

        // Get the frame data for the current frame in the ring buffer
        auto& frame = m_frameData[m_frameRingCurrent];

        // Wait until GPU has finished processing the frame that was using these resources previously (numFramesInFlight frames ago)
        const uint64_t waitValue = frame.frameNumber;
        const VkSemaphoreWaitInfo waitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &m_frameTimelineSemaphore,
            .pValues = &waitValue,
        };
        vkWaitSemaphores(device, &waitInfo, std::numeric_limits<uint64_t>::max());

        /*--
         * Reset the command pool to reuse the command buffer for recording
         * new rendering commands for the current frame.
        -*/
        VK_CHECK(vkResetCommandPool(device, frame.cmdPool, 0));
        VkCommandBuffer cmd = frame.cmdBuffer;

        // Begin the command buffer recording for the frame
        const VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

        // Acquire the image to render into
        VkResult result = m_swapchain.acquireNextImage(device);
        if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
            return cmd;

        return {};
    }

    VanKCommandBuffer VulkanRendererAPI::BeginCommandBuffer()
    {
        VkCommandBuffer vkCmd = beginFrame(); // your existing Vulkan call
        if (vkCmd == VK_NULL_HANDLE)
            return nullptr;

        DBG_VK_SCOPE(vkCmd); // <-- Helps to debug in NSight

        return new VanKCommandBuffer_T{vkCmd};
    }

    /*--
       * End the frame by submitting the command buffer to the GPU and presenting the image.
       * Adds binary semaphores to wait for the image to be available and signal when rendering is done.
       * Adds the timeline semaphore to signal when the frame is completed.
       * Moves to the next frame.
      -*/

    void VulkanRendererAPI::endFrame(VanKCommandBuffer cmd)
    {
        // Ends recording of commands for the frame
        VK_CHECK(vkEndCommandBuffer(Unwrap(cmd)));

        /*-- 
         * Prepare to submit the current frame for rendering 
         * First add the swapchain semaphore to wait for the image to be available.
        -*/
        std::vector<VkSemaphoreSubmitInfo> waitSemaphores;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphores;
        waitSemaphores.push_back({
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_swapchain.getWaitSemaphores(),
            .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        });
        signalSemaphores.push_back({
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_swapchain.getSignalSemaphores(),
            .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        });

        // Get the frame data for the current frame in the ring buffer
        auto& frame = m_frameData[m_frameRingCurrent];

        /*--
         * Calculate the signal value for when this frame completes
         * Signal value = current frame number + numFramesInFlight
         * Example with 3 frames in flight:
         *   Frame 0 signals value 3 (allowing Frame 3 to start when complete)
         *   Frame 1 signals value 4 (allowing Frame 4 to start when complete)
        -*/
        const uint64_t signalFrameValue = frame.frameNumber + m_swapchain.getMaxFramesInFlight();
        frame.frameNumber = signalFrameValue; // Store for next time this frame buffer is used

        /*-- 
         * Add timeline semaphore to signal when GPU completes this frame
         * The color attachment output stage is used since that's when the frame is fully rendered
        -*/
        signalSemaphores.push_back({
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_frameTimelineSemaphore,
            .value = signalFrameValue,
            .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        });

        // Note : in this sample, we only have one command buffer per frame.
        const std::array<VkCommandBufferSubmitInfo, 1> cmdBufferInfo{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .commandBuffer = Unwrap(cmd),
                }
            }
        };

        // Populate the submit info to synchronize rendering and send the command buffer
        const std::array<VkSubmitInfo2, 1> submitInfo{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                    .waitSemaphoreInfoCount = uint32_t(waitSemaphores.size()), //
                    .pWaitSemaphoreInfos = waitSemaphores.data(), // Wait for the image to be available
                    .commandBufferInfoCount = uint32_t(cmdBufferInfo.size()), //
                    .pCommandBufferInfos = cmdBufferInfo.data(), // Command buffer to submit
                    .signalSemaphoreInfoCount = uint32_t(signalSemaphores.size()), //
                    .pSignalSemaphoreInfos = signalSemaphores.data(), // Signal when rendering is finished
                }
            }
        };

        // Submit the command buffer to the GPU and signal when it's done
        VK_CHECK(
            vkQueueSubmit2(m_context.getGraphicsQueue().queue, uint32_t(submitInfo.size()), submitInfo.data(), nullptr));

        // Present the image
        m_swapchain.presentFrame(m_context.getGraphicsQueue().queue);

        // Move to the next frame
        m_frameRingCurrent = (m_frameRingCurrent + 1) % m_swapchain.getMaxFramesInFlight();
    }

    /*-- 
       * Call this function if the viewport size changes 
       * This happens when the window is resized, or when the ImGui viewport window is resized.
      -*/

    void VulkanRendererAPI::OnViewportSizeChange(const Extent2D& size)
    {
        VkExtent2D vkSize = {size.width, size.height};

        m_viewportSize = vkSize;
        // Recreate the G-Buffer to the size of the viewport
        vkQueueWaitIdle(m_context.getGraphicsQueue().queue);
        {
            VkCommandBuffer cmd = utils::beginSingleTimeCommands(m_context.getDevice(), m_transientCmdPool);
            m_gBuffer.update(cmd, m_viewportSize);
            utils::endSingleTimeCommands(cmd, m_context.getDevice(), m_transientCmdPool,
                                         m_context.getGraphicsQueue().queue);
        }
    }

    /*--
       * We are using dynamic rendering, which is a more flexible way to render to the swapchain image.
      -*/

    void VulkanRendererAPI::beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd)
    {
        // Image to render to
        VkRenderingAttachmentInfoKHR colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_swapchain.getNextImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear the image (see clearValue)
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Store the image (keep the image)
            .clearValue = {{{0.2f, 0.2f, 0.3f, 1.0f}}}, // Same background color as G-Buffer
        };

        // Details of the dynamic rendering
        const VkRenderingInfoKHR renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = {{0, 0}, m_windowSize},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        // Transition the swapchain image to the color attachment layout, needed when using dynamic rendering
        utils::cmdTransitionImageLayout(Unwrap(cmd), m_swapchain.getNextImage(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        vkCmdBeginRendering(Unwrap(cmd), &renderingInfo);
    }

    /*--
       * End of dynamic rendering.
       * The image is transitioned back to the present layout, and the rendering is ended.
      -*/

    void VulkanRendererAPI::endDynamicRenderingToSwapchain(VanKCommandBuffer cmd)
    {
        vkCmdEndRendering(Unwrap(cmd));

        // Transition the swapchain image back to the present layout
        utils::cmdTransitionImageLayout(Unwrap(cmd), m_swapchain.getNextImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    /*-- 
       * This is invoking the compute shader to update the Vertex in the buffer (animation of triangle).
       * Where those vertices will be then used to draw the geometry
      -*/
    
    void VulkanRendererAPI::DispatchCompute(VanKComputePass* computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        // Execute the compute shader
        // The workgroup is set to 256, and we only have 3 vertex to deal with, so one group is enough
        vkCmdDispatch(Unwrap(computePass->VanKCommandBuffer), groupCountX, groupCountY, groupCountZ);
    }

    VkShaderStageFlags GetShaderStageFlags(VanKPipelineBindPoint bindPoint)
    {
        return static_cast<VkShaderStageFlags>((bindPoint == VanKPipelineBindPoint::Compute) ? 
               VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS);
    }

    void VulkanRendererAPI::BindStorageBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, VanKBuffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        if (!buffer)
        {
            return;
        }

        // Get the Vulkan buffer handle properly
        VkBuffer vkBuffer = VK_NULL_HANDLE;
    
        // Try to cast to different buffer types and get the actual VkBuffer handle
        if (auto* storageBuffer = dynamic_cast<VulkanStorageBuffer*>(buffer))
        {
            vkBuffer = storageBuffer->GetBuffer().buffer;
        }
        else if (auto* vertexBuffer = dynamic_cast<VulkanVertexBuffer*>(buffer))
        {
            vkBuffer = vertexBuffer->GetBuffer().buffer;
        }
        else if (auto* indexBuffer = dynamic_cast<VulkanIndexBuffer*>(buffer))
        {
            vkBuffer = indexBuffer->GetBuffer().buffer;
        }
        else if (auto* uniformBuffer = dynamic_cast<VulkanUniformBuffer*>(buffer))
        {
            vkBuffer = uniformBuffer->GetBuffer().buffer;
        }
    
        if (vkBuffer == VK_NULL_HANDLE)
        {
            // Handle error: invalid buffer type or buffer not created properly
            return;
        }
    
        // Setting up push descriptor information for storage buffer
        const VkDescriptorBufferInfo bufferInfo = {.buffer = vkBuffer, .offset = 0, .range = VK_WHOLE_SIZE};
        const std::array<VkWriteDescriptorSet, 1> writeDescriptorSet = {
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = nullptr, // Not set, this is a push descriptor
                    .dstBinding = binding, // layout(binding = X) in the shader
                    .dstArrayElement = arrayElement, // If we were to use an array of buffers
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Key difference from uniform buffer
                    .pBufferInfo = &bufferInfo,
                }
            }
        };

        // Push layout information with updated data
        const VkPushDescriptorSetInfoKHR pushDescriptorSetInfo{
            .sType = VK_STRUCTURE_TYPE_PUSH_DESCRIPTOR_SET_INFO_KHR,
            .stageFlags = GetShaderStageFlags(bindPoint),
            .layout = (bindPoint == VanKPipelineBindPoint::Compute) ? 
                      m_computePipelineLayout : m_graphicPipelineLayout,
            .set = set, // Descriptor set number
            .descriptorWriteCount = uint32_t(writeDescriptorSet.size()),
            .pDescriptorWrites = writeDescriptorSet.data(),
        };
    
        // This is a push descriptor, allowing synchronization and dynamically changing data
        vkCmdPushDescriptorSet2(Unwrap(cmd), &pushDescriptorSetInfo);
    }

    void VulkanRendererAPI::BindUniformBuffer(VanKCommandBuffer cmd, VanKPipelineBindPoint bindPoint, UniformBuffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayElement = 0)
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;

        if (bindPoint == VanKPipelineBindPoint::Graphics) layout = m_currentGraphicsPipelineLayout;
        if (bindPoint == VanKPipelineBindPoint::Compute) layout = m_currentComputePipelineLayout;
    
        VulkanUniformBuffer* vulkanUBO = dynamic_cast<VulkanUniformBuffer*>(buffer);
        if (!vulkanUBO)
        {
            return;
        }

        const utils::Buffer& vkBuffer = vulkanUBO->GetBuffer();
    
        // Setting up push descriptor information, we could choose dynamically the buffer to work on
        const VkDescriptorBufferInfo bufferInfo = {.buffer = vkBuffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE};
        const std::array<VkWriteDescriptorSet, 1> writeDescriptorSet = {
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = nullptr, // Not set, this is a push descriptor
                    .dstBinding = binding, // layout(binding = 0) in the fragment shader
                    .dstArrayElement = arrayElement, // If we were to use an array of images
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                }
            }
        };

        // Push layout information with updated data
        const VkPushDescriptorSetInfoKHR pushDescriptorSetInfo{
            .sType = VK_STRUCTURE_TYPE_PUSH_DESCRIPTOR_SET_INFO_KHR,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .layout = m_graphicPipelineLayout,
            .set = set, // <--- Second set layout(set=1, binding=...) in the fragment shader
            .descriptorWriteCount = uint32_t(writeDescriptorSet.size()),
            .pDescriptorWrites = writeDescriptorSet.data(),
        };
    
        // This is a push descriptor, allowing synchronization and dynamically changing data
        vkCmdPushDescriptorSet2(Unwrap(cmd), &pushDescriptorSetInfo);
    }

    void VulkanRendererAPI::BeginRendering(VanKCommandBuffer cmd, const VanKColorTargetInfo* color_target_info, uint32_t num_color_targets, VanKDepthStencilTargetInfo depth_stencil_target_info)
    {
        m_num_color_targets = num_color_targets;
    
        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        colorAttachments.reserve(num_color_targets);
        for (size_t i = 0; i < num_color_targets; i++)
        {
            const auto& info = color_target_info[i];
        
            VkRenderingAttachmentInfoKHR colorAttachment {};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            colorAttachment.imageView = m_gBuffer.getColorImageView(i);
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            colorAttachment.loadOp = ConvertToVkLoadOp(info.loadOp);
            colorAttachment.storeOp = ConvertToVkStoreOp(info.storeOp);
            colorAttachment.clearValue.color = ConvertToVkClearColor(info.clearColor, ConvertToVkFormat(info.format));

            colorAttachments.emplace_back(colorAttachment);
        }
   
        // Depth buffer to use
        const VkRenderingAttachmentInfoKHR depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_gBuffer.getDepthImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
            .loadOp = ConvertToVkLoadOp(depth_stencil_target_info.loadOp), // Clear depth buffer
            .storeOp = ConvertToVkStoreOp(depth_stencil_target_info.storeOp), // Store depth buffer
            .clearValue = { .depthStencil = ConvertToVkClearDepthStencil(depth_stencil_target_info.clearColor) },
        };

        // Details of the dynamic rendering
        const VkRenderingInfoKHR renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = {{0, 0}, m_gBuffer.getSize()},
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
            .pColorAttachments = colorAttachments.data(),
            .pDepthAttachment = &depthAttachment,
        };

        /*-- 
         * Transition the swapchain image to the color attachment layout, needed when using dynamic rendering 
         -*/
    
        for (size_t i = 0; i < num_color_targets; ++i) {
            utils::cmdTransitionImageLayout(Unwrap(cmd), m_gBuffer.getColorImage(i), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    
        vkCmdBeginRendering(Unwrap(cmd), &renderingInfo);
    }

    void VulkanRendererAPI::SetViewport(VanKCommandBuffer cmd, uint32_t viewportCount, const VanKViewport viewport)
    {
        VkViewport vkViewport{viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
        vkCmdSetViewportWithCount(Unwrap(cmd), viewportCount, &vkViewport);
    }

    void VulkanRendererAPI::SetScissor(VanKCommandBuffer cmd, uint32_t scissorCount, const VankRect scissor)
    {
        VkRect2D vkScissor{{scissor.x, scissor.y}, {scissor.width, scissor.height}};
        vkCmdSetScissorWithCount(Unwrap(cmd), scissorCount, &vkScissor);
    }

    void VulkanRendererAPI::SetLineWidth(VanKCommandBuffer cmd, float LineWidth)
    {
        vkCmdSetLineWidth(Unwrap(cmd), LineWidth);
    }

    void VulkanRendererAPI::BindFragmentSamplers(VanKCommandBuffer cmd, uint32_t firstSlot, const TextureSamplerBinding* samplers, uint32_t num_bindings)
    {
        //might have to change this i tryed putting updatedescriptor set here but idk do i need this in bindless ?
        /*-- 
         * Binding the resources passed to the shader, using the descriptor set (holds the texture) 
         * There are two descriptor layouts, one for the texture and one for the scene information,
         * but only the texture is a set, the scene information is a push descriptor.
        -*/
        const VkBindDescriptorSetsInfoKHR bindDescriptorSetsInfo = {
            .sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .layout = m_graphicPipelineLayout,
            .firstSet = 0,
            .descriptorSetCount = 1,
            .pDescriptorSets = &m_textureDescriptorSet,
        };
        vkCmdBindDescriptorSets2(Unwrap(cmd), &bindDescriptorSetsInfo);
    }

    void VulkanRendererAPI::BindVertexBuffer(VanKCommandBuffer cmd, uint32_t first_slot, const VertexBuffer& vertexBuffer, uint32_t num_bindings)
    {
        // Cast to VulkanVertexBuffer
        const VulkanVertexBuffer* vulkanVB = dynamic_cast<const VulkanVertexBuffer*>(&vertexBuffer);
        if (!vulkanVB) {
            // Handle error: wrong buffer type
            return;
        }

        const utils::Buffer& vkBuffer = vulkanVB->GetBuffer();
        VkBuffer buffer = vkBuffer.buffer; // The actual VkBuffer

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(Unwrap(cmd), first_slot, num_bindings, &buffer, offsets);
    }

    void VulkanRendererAPI::BindIndexBuffer(VanKCommandBuffer cmd, const IndexBuffer& indexBuffer, VanKIndexElementSize elementSize)
    {
        // Cast to VulkanVertexBuffer
        const VulkanIndexBuffer* vulkanIB = dynamic_cast<const VulkanIndexBuffer*>(&indexBuffer);
        if (!vulkanIB) {
            // Handle error: wrong buffer type
            return;
        }

        const utils::Buffer& vkBuffer = vulkanIB->GetBuffer();
        VkBuffer buffer = vkBuffer.buffer; // The actual VkBuffer

        VkIndexType vkIndexType = VK_INDEX_TYPE_MAX_ENUM;
        switch (elementSize)
        {
        case VanKIndexElementSize::Uint16: vkIndexType = VK_INDEX_TYPE_UINT16; break;
        case VanKIndexElementSize::Uint32: vkIndexType = VK_INDEX_TYPE_UINT32; break;
        }
    
        vkCmdBindIndexBuffer(Unwrap(cmd), buffer, 0, vkIndexType);
    }

    void VulkanRendererAPI::PushConstants(VanKCommandBuffer cmd, VanKShaderStageFlags stageFlags, uint32_t slot_index, const void* data, uint32_t length)
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (ConvertToVkShaderStageFlagBits(stageFlags) == VK_SHADER_STAGE_ALL_GRAPHICS)
        {
            layout = m_currentGraphicsPipelineLayout;
        }
        if (ConvertToVkShaderStageFlagBits(stageFlags) == VK_SHADER_STAGE_COMPUTE_BIT)
        {
            layout = m_currentComputePipelineLayout;
        }
        // Push constant information, see usage later
        const VkPushConstantsInfoKHR pushInfo{
            .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
            .layout = layout,
            .stageFlags = ConvertToVkShaderStageFlagBits(stageFlags),
            .offset = slot_index,// push constant is byte offset but in uniform its bindingslot
            .size = length,
            .pValues = data,
        };
    
        vkCmdPushConstants2(Unwrap(cmd), &pushInfo);
    }

    void VulkanRendererAPI::BindPipeline(VanKCommandBuffer cmd, VanKPipelineBindPoint pipelineBindPoint,
                                         VanKPipeLine pipeline)
    {
        VkPipeline pipelineToBind = VK_NULL_HANDLE;
        VkPipelineBindPoint vkBindPoint = {};
        VkPipelineLayout pipelineLayoutToBind = VK_NULL_HANDLE;
    
        if (pipelineBindPoint == VanKPipelineBindPoint::Graphics)
        {
            vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            // Use the 'pipelineType' PARAMETER as the key
            
            pipelineToBind = Unwrap(pipeline);
            pipelineLayoutToBind = m_graphicPipelineLayout;
        }
        else // Compute
        {
            vkBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            // Use the 'pipelineType' PARAMETER as the key
            
            pipelineToBind = Unwrap(pipeline);
            pipelineLayoutToBind = m_computePipelineLayout;
        }

        if (pipelineToBind != VK_NULL_HANDLE)
        {
            vkCmdBindPipeline(Unwrap(cmd), vkBindPoint, pipelineToBind);
            // Only update the relevant current pipeline layout
            if (pipelineBindPoint == VanKPipelineBindPoint::Graphics)
            {
                m_currentGraphicsPipelineLayout = pipelineLayoutToBind;
            }
            else
            {
                m_currentComputePipelineLayout = pipelineLayoutToBind;
            }
        }
        else
        {
            // Handle error: pipeline not found
        }
    }

    void VulkanRendererAPI::DrawIndexed(VanKCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount,
                                        uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(Unwrap(cmd), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanRendererAPI::Draw(VanKCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(Unwrap(cmd), vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanRendererAPI::EndRendering(VanKCommandBuffer cmd)
    {
        vkCmdEndRendering(Unwrap(cmd));

        for (size_t i = 0; i < m_num_color_targets; ++i) {
            utils::cmdTransitionImageLayout(Unwrap(cmd), m_gBuffer.getColorImage(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        }
    }

    /*--
       * The graphic pipeline is all the stages that are used to render a section of the scene.
       * Stages like: vertex shader, fragment shader, rasterization, and blending.
    --*/

    struct VertexInputDescription
    {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
        case ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
        case ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
        case ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
        case ShaderDataType::Bool: return VK_FORMAT_R8_UINT; // or VK_FORMAT_R32_UINT if you want 4 bytes
            // Add Mat3/Mat4 if you want to support them as arrays of vec3/vec4
        default: return VK_FORMAT_UNDEFINED;
        }
    }

    inline VertexInputDescription BufferLayoutToVertexInput(const BufferLayout& layout, uint32_t binding = 0)
    {
        VertexInputDescription desc;
        desc.bindings.push_back({
            binding,
            layout.GetStride(),
            VK_VERTEX_INPUT_RATE_VERTEX
        });

        uint32_t location = 0;
        for (const auto& element : layout.GetElements())
        {
            desc.attributes.push_back({
                location++,
                binding,
                ShaderDataTypeToVulkanFormat(element.Type),
                element.Offset
            });
        }
        return desc;
    }

    VanKPipeLine VulkanRendererAPI::createGraphicsPipeline(VanKGraphicsPipelineSpecification pipelineSpecification)
    {
        auto specShader = pipelineSpecification.ShaderStageCreateInfo.VanKShader;
        auto vkShader = dynamic_cast<const VulkanShader*>(specShader);
        
        pipelineSpecifications = pipelineSpecification;
        m_graphicsShader = vkShader;
        
        std::string vertShaderEntryName = vkShader->GetShaderEntryName(VK_SHADER_STAGE_VERTEX_BIT);
        std::string fragShaderEntryName = vkShader->GetShaderEntryName(VK_SHADER_STAGE_FRAGMENT_BIT);
        VkShaderModule vertShaderModule = vkShader->GetShaderModule(VK_SHADER_STAGE_VERTEX_BIT);
        VkShaderModule fragShaderModule = vkShader->GetShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT);
        
        /*--  
         * Define specialization constants for the fragment shader
         * Not required, but used in this sample, as it's a good example of how to use specialization constants.
        -*/
        const std::optional<VanKSpecializationInfo>& pipelineSpecialization = pipelineSpecification.ShaderStageCreateInfo.specializationInfo;
        
        VkSpecializationInfo vkSpecInfo{};
        std::vector<VkSpecializationMapEntry> vkEntries;
        const VkSpecializationInfo* pSpecializationInfo = nullptr;

        if (pipelineSpecialization.has_value())
        {
            const auto& val = pipelineSpecialization.value();

            for (const auto& entry : val.MapEntries)
            {
                vkEntries.push_back({ entry.constantID, entry.offset, static_cast<uint32_t>(entry.size) });
            }

            vkSpecInfo.mapEntryCount = static_cast<uint32_t>(vkEntries.size());
            vkSpecInfo.pMapEntries = vkEntries.data();
            vkSpecInfo.dataSize = static_cast<uint32_t>(val.dataSize());
            vkSpecInfo.pData = val.getData();

            // <-- This is the only thing Vulkan wants.
            pSpecializationInfo = &vkSpecInfo;
        }

        // The stages used by this pipeline
        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
            {
                {
                    // Vertex shader
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertShaderModule,
                    .pName = vertShaderEntryName.c_str(),
                },
                {
                    // Fragment shader
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragShaderModule,
                    .pName = fragShaderEntryName.c_str(),
                    .pSpecializationInfo = &vkSpecInfo
                },
            }
        };

        auto specBuffer = pipelineSpecification.VertexInputStateCreateInfo.VanKBuffer;
        auto vertexInput = BufferLayoutToVertexInput(specBuffer->GetLayout());
    
        // Describe the layout of the Vertex in the Buffer, which is passed to the vertex shader
        const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = uint32_t(vertexInput.bindings.size()),
            .pVertexBindingDescriptions = vertexInput.bindings.data(),
            .vertexAttributeDescriptionCount = uint32_t(vertexInput.attributes.size()),
            .pVertexAttributeDescriptions = vertexInput.attributes.data(),
        };

        // The input assembly is used to describe how the vertices are assembled into primitives (triangles)
        const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = ConvertToVkPrimitiveTopology(pipelineSpecification.InputAssemblyStateCreateInfo.VanKPrimitive),
            .primitiveRestartEnable = VK_FALSE,
        };

        /*--
         * The dynamic state is used to change the viewport and the scissor dynamically.
         * If we don't do this, we need to recreate the pipeline when the window is resized.
         * NOTE: more dynamic states can be added, but performance 'can' be impacted.
        -*/
        const std::array<VkDynamicState, 3> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
            VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = uint32_t(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        // The rasterizer is used to convert the primitives into fragments, and how it will appear
        const VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = ConvertToVkPolygonMode(pipelineSpecification.RasterizationStateCreateInfo.VanKPolygon),
            .cullMode = ConvertToVkCullMode(pipelineSpecification.RasterizationStateCreateInfo.VanKCullMode), // No culling
            .frontFace = ConvertToVkFrontFace(pipelineSpecification.RasterizationStateCreateInfo.VanKFrontFace),
        };

        // No multi-sampling
        const VkPipelineMultisampleStateCreateInfo multisamplingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        /*--
         * The color blending is used to blend the color of the fragment with the color already in the framebuffer (all channel)
         * Here we enable blending, such that the alpha channel is used to blend the color with the color already in the framebuffer.
         * The texture will have part transparent.
         *
         * Without blending, everything can be set to 0, except colorWriteMask, which needs to be set.
        -*/
        const std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments = {
            {
                {
                    .blendEnable = VK_TRUE,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT,
                },
                {
                    .blendEnable = VK_FALSE, // No blending for entity ID buffer
                    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT, // Only R is used for R32_SINT
                }
            }
        };

        const VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE, // No logic operation
            .logicOp = VK_LOGIC_OP_COPY, // Don't care
            .attachmentCount = uint32_t(colorBlendAttachments.size()),
            .pAttachments = colorBlendAttachments.data(),
        };

        // Push constant is used to pass data to the shader at each frame
        const VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .offset = 0,
            .size = sizeof(shaderio::PushConstant),
        };

        // The two layout to use
        const std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
            {
                m_textureDescriptorSetLayout, // All application textures
                m_commonDescriptorSetLayout, // The scene information, and more eventually
            }
        };

        // The pipeline layout is used to pass data to the pipeline, anything with "layout" in the shader
        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = uint32_t(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(), // The descriptor set layout
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
        };
        VK_CHECK(vkCreatePipelineLayout(m_context.getDevice(), &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayout));
        DBG_VK_NAME(m_graphicPipelineLayout);

        // Dynamic rendering: provide what the pipeline will render to
        const std::array<VkFormat, 2> imageFormats = {
            {
                m_gBuffer.getColorFormat(0),
                m_gBuffer.getColorFormat(1)
            }
        };
        
        const VkPipelineRenderingCreateInfo dynamicRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = uint32_t(imageFormats.size()),
            .pColorAttachmentFormats = imageFormats.data(),
            .depthAttachmentFormat = m_gBuffer.getDepthFormat(),
        };

        // Instruct how the depth buffer will be used
        const VkPipelineDepthStencilStateCreateInfo depthStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        };

        // The pipeline is created with all the information
        const VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &dynamicRenderingInfo,
            .stageCount = uint32_t(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            //.pViewportState      = DYNAMIC see above,
            .pRasterizationState = &rasterizerInfo,
            .pMultisampleState = &multisamplingInfo,
            .pDepthStencilState = &depthStateInfo,
            .pColorBlendState = &colorBlendingInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = m_graphicPipelineLayout,
        };
        VK_CHECK(
            vkCreateGraphicsPipelines(m_context.getDevice(), nullptr, 1, &pipelineInfo, nullptr, &
                m_graphicsPipeline));
        DBG_VK_NAME(m_graphicsPipeline);

        m_currentGraphicsPipelineLayout = m_graphicPipelineLayout;

        m_graphicsPipelines.push_back(m_graphicsPipeline);
        
        m_graphicsPipelinesLayouts.push_back(m_graphicPipelineLayout);
        
        return Wrap(m_graphicsPipeline);
    }

    /*-- Initialize ImGui -*/

    void VulkanRendererAPI::initImGui()
    {
        /*IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();*/

        createImGuiDescriptorPool();

        ImGui_ImplSDL3_InitForVulkan(m_window);
        static VkFormat imageFormats[] = {m_swapchain.getImageFormat()};
        ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = m_context.getInstance(),
            .PhysicalDevice = m_context.getPhysicalDevice(),
            .Device = m_context.getDevice(),
            .QueueFamily = m_context.getGraphicsQueue().familyIndex,
            .Queue = m_context.getGraphicsQueue().queue,
            .DescriptorPool = m_imguiDescriptorPool,
            .MinImageCount = 2,
            .ImageCount = m_swapchain.getMaxFramesInFlight(),
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = // Dynamic rendering
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = imageFormats,
            },
        };

        ImGui_ImplVulkan_Init(&initInfo);

        ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        m_gBuffer.updateImGuiDescriptors();
    }

    /*-- Render ImGui -*/
    void VulkanRendererAPI::renderImGui(VanKCommandBuffer cmd)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Unwrap(cmd));
    }

    /*-- Blit SwapChain --*/
    void VulkanRendererAPI::BlitGBufferToSwapchain(VanKCommandBuffer cmd)
    {
        // --- Transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ---
        utils::cmdTransitionImageLayout(
            Unwrap(cmd),
            m_swapchain.getNextImage(),
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        // --- Transition GBuffer image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ---
        utils::cmdTransitionImageLayout(
            Unwrap(cmd),
            m_gBuffer.getColorImage(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );

        // --- Blit ---
        VkImageBlit2 blitRegion{
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
            .srcOffsets = {{0, 0, 0}, {int32_t(m_viewportSize.width), int32_t(m_viewportSize.height), 1}},
            .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
            .dstOffsets = {{0, 0, 0}, {int32_t(m_windowSize.width), int32_t(m_windowSize.height), 1}}
        };
        VkBlitImageInfo2 blitInfo{
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .srcImage = m_gBuffer.getColorImage(),
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = m_swapchain.getNextImage(),
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &blitRegion,
            .filter = VK_FILTER_NEAREST
        };
        vkCmdBlitImage2(Unwrap(cmd), &blitInfo);

        // --- Transition images back to their original layouts ---
        utils::cmdTransitionImageLayout(
            Unwrap(cmd),
            m_swapchain.getNextImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
        utils::cmdTransitionImageLayout(
            Unwrap(cmd),
            m_gBuffer.getColorImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL
        );
    }

    /*--
       * The Descriptor Pool is used to allocate descriptor sets.
       * Currently, only ImGui requires a combined image sampler.
      -*/

    void VulkanRendererAPI::createDescriptorPool()
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_context.getPhysicalDevice(), &deviceProperties);

        // We don't need to set the exact number of descriptor sets, but we need to set a maximum
        const uint32_t safegardSize = 2;
        uint32_t maxDescriptorSets = std::min(1000U, deviceProperties.limits.maxDescriptorSetUniformBuffers - safegardSize);
        m_maxTextures = std::min(m_maxTextures, deviceProperties.limits.maxDescriptorSetSampledImages - safegardSize);

        const std::array<VkDescriptorPoolSize, 1> poolSizes{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_maxTextures},
        };

        const VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |
            //  allows descriptor sets to be updated after they have been bound to a command buffer
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            // individual descriptor sets can be freed from the descriptor pool
            .maxSets = maxDescriptorSets, // Allowing to create many sets (ImGui uses this for textures)
            .poolSizeCount = uint32_t(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };
        VK_CHECK(vkCreateDescriptorPool(m_context.getDevice(), &poolInfo, nullptr, &m_descriptorPool));
        DBG_VK_NAME(m_descriptorPool);
    }

    void VulkanRendererAPI::createImGuiDescriptorPool()
    {
        size_t gbufferTextures = m_gBuffer.getNumImGuiTextures(); // or whatever your GBuffer uses
        size_t otherImGuiTextures = Texture2D::GetNumImGuiTextures(); // estimate or track other ImGui::Image() textures
        size_t totalImGuiTextures = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + gbufferTextures + otherImGuiTextures + 100;
        // Create separate descriptor pool for ImGui
        std::array<VkDescriptorPoolSize, 1> imguiPoolSizes{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  static_cast<uint32_t>(totalImGuiTextures)}, // ImGui needs lots of texture descriptors
        };

        VkDescriptorPoolCreateInfo imguiPoolInfo = {};
        imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        for (VkDescriptorPoolSize& pool_size : imguiPoolSizes)
            imguiPoolInfo.maxSets += pool_size.descriptorCount; // ImGui creates many descriptor sets
        imguiPoolInfo.poolSizeCount = uint32_t(imguiPoolSizes.size());
        imguiPoolInfo.pPoolSizes = imguiPoolSizes.data();
        
        VK_CHECK(vkCreateDescriptorPool(m_context.getDevice(), &imguiPoolInfo, nullptr, &m_imguiDescriptorPool));
        DBG_VK_NAME(m_imguiDescriptorPool);
    }

    /*--
       * The Vulkan descriptor set defines the resources that are used by the shaders.
       * In this application we have two descriptor layout, one for the texture and one for the scene information.
       * But only one descriptor set (texture), the scene information is a push descriptor.
       * Set are used to group resources, and layout to define the resources.
       * Push are limited to a certain number of bindings, but are synchronized with the frame.
       * Set can be huge, but are not synchronized with the frame (command buffer).
       -*/

    void VulkanRendererAPI::createGraphicDescriptorSet()
    {
        // First describe the layout of the texture descriptor, what and how many
        {
            uint32_t numTextures = m_maxTextures; // We don't need to set the exact number of texture the scene have.

            // In comment, the layout for a storage buffer, which is not used in this sample, but rather a push descriptor (below)
            const std::array<VkDescriptorSetLayoutBinding, 1> layoutBindings{
                {
                    {
                        .binding = shaderio::LBindTextures,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = numTextures,
                        .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
                    },
                
                    // This is if we would add another binding for the scene info, but instead we make another set, see below
                    // {.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
                }
            };

            const VkDescriptorBindingFlags flags[] = {
                // Flags for binding 0 (texture array):
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | // Can update while in use
                VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | // Can update unused entries
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, // Not all array elements need to be valid (0,2,3 vs 0,1,2,3)

                // Flags for binding 1 (scene info buffer):
                // VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT  // flags for storage buffer binding
            };
            const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                .bindingCount = uint32_t(layoutBindings.size()), // matches our number of bindings
                .pBindingFlags = flags, // the flags for each binding
            };

            const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = &bindingFlags,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
                // Allows to update the descriptor set after it has been bound
                .bindingCount = uint32_t(layoutBindings.size()),
                .pBindings = layoutBindings.data(),
            };
            VK_CHECK(
                vkCreateDescriptorSetLayout(m_context.getDevice(), &descriptorSetLayoutInfo, nullptr, &
                    m_textureDescriptorSetLayout));
            DBG_VK_NAME(m_textureDescriptorSetLayout);

            // Allocate the descriptor set, needed only for larger descriptor sets
            const VkDescriptorSetAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_textureDescriptorSetLayout,
            };
            VK_CHECK(vkAllocateDescriptorSets(m_context.getDevice(), &allocInfo, &m_textureDescriptorSet));
            DBG_VK_NAME(m_textureDescriptorSet);
        }

        // Second this is another set which will be pushed
        {
            // This is the scene buffer information
            const std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings{
            {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT // Only used in compute
                }
                ,
                {
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT // Only used in compute
                }
            }
            };
            const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
                .bindingCount = uint32_t(layoutBindings.size()),
                .pBindings = layoutBindings.data(),
            };
            VK_CHECK(
                vkCreateDescriptorSetLayout(m_context.getDevice(), &descriptorSetLayoutInfo, nullptr, &
                    m_commonDescriptorSetLayout));
            DBG_VK_NAME(m_commonDescriptorSetLayout);
        }
    }

    /*--
       * The resources associated with the descriptor set must be set, in order to be used in the shaders.
       * This is actually updating the unbind array of textures
      -*/

    void VulkanRendererAPI::updateGraphicsDescriptorSet()
    {
        // The sampler used for the texture
        const VkSampler sampler = m_samplerPool.acquireSampler({
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .maxLod = VK_LOD_CLAMP_NONE,
        });
        DBG_VK_NAME(sampler);

        // Prepare imageInfos vector automatically sized to m_image's size
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(m_image.size()); // reserve for efficiency

        // The image info
        for (size_t i = 0; i < m_image.size(); ++i)
        {
            imageInfos.push_back({
                .sampler = sampler,
                .imageView = m_image[i].view,
                .imageLayout = m_image[m_imageID].layout,
            });
        }

        std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_textureDescriptorSet, // Only needed if we are using a descriptor set, not push descriptor
                    .dstBinding = shaderio::LBindTextures, // layout(binding = 0) in the fragment shader
                    .dstArrayElement = 0, // If we were to use an array of images
                    .descriptorCount = uint32_t(imageInfos.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = imageInfos.data(),
                }
            }
        };

        // This is if the scene info buffer if part of the descriptor set layout (we have it in a separate set/layout)
        // VkDescriptorBufferInfo bufferInfo = {.buffer = m_sceneInfoBuffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE};
        // writeDescriptorSets.push_back({
        //     .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //     .dstSet          = m_textureDescriptorSet,  // Not set, this is a push descriptor
        //     .dstBinding      = 1,                       // layout(binding = 1) in the fragment shader
        //     .dstArrayElement = 0,                       // If we were to use an array of images
        //     .descriptorCount = 1,
        //     .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        //     .pBufferInfo     = &bufferInfo,
        // });

        /*-- 
         * With the flags set it ACTUALLY allows:
         *  - You can update after binding to a command buffer but before submitting.
         *  - You can update while the descriptor set is bound in another thread.
         *  - You don't invalidate the command buffer when you update.
         *  - Multiple threads can update different descriptors at the same time
         * What it does NOT allow:
         *  - Update while the GPU is actively reading it in a shader
         *  - Skipping proper synchronization between CPU updates and GPU reads
         *  - Simultaneous updates to the same descriptor
         * Since this is called before starting to render, we don't need to worry about the first two.
        -*/
        vkUpdateDescriptorSets(m_context.getDevice(), uint32_t(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                               nullptr);
    }

    /*--
       * Loading an image using the stb_image library.
       * Create an image and upload the data to the GPU.
       * Create an image view (Image view are for the shaders, to access the image).
      -*/

    utils::ImageResource VulkanRendererAPI::loadAndCreateImage(VkCommandBuffer cmd, const std::string& filename)
    {
        stbi_set_flip_vertically_on_load(true); // Flip the image vertically

        // Load the image from disk
        int w, h, comp, req_comp{4};
        stbi_uc* data = stbi_load(filename.c_str(), &w, &h, &comp, req_comp);
        ASSERT(data != nullptr, "Could not load texture image!");
        const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        // Define how to create the image
        const VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {uint32_t(w), uint32_t(h), 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        };

        // Use the VMA allocator to create the image
        const std::span dataSpan(data, w * h * 4);
        utils::ImageResource image =
            m_allocator.createImageAndUploadData(cmd, dataSpan, imageInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        DBG_VK_NAME(image.image);
        image.extent = {uint32_t(w), uint32_t(h)};

        // Create the image view
        const VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
        };
        VK_CHECK(vkCreateImageView(m_context.getDevice(), &viewInfo, nullptr, &image.view));
        DBG_VK_NAME(image.view);

        stbi_image_free(data);

        return image;
    }

    // Creating the compute shader pipeline

    VanKPipeLine VulkanRendererAPI::createComputeShaderPipeline(
        VanKComputePipelineSpecification computePipelineSpecification)
    {
        auto specShader = dynamic_cast<const VulkanShader*>(computePipelineSpecification.ComputePipelineCreateInfo.VanKShader);
        std::string computeEntryName = specShader->GetShaderEntryName(VK_SHADER_STAGE_COMPUTE_BIT);
        VkShaderModule compute = specShader->GetShaderModule(VK_SHADER_STAGE_COMPUTE_BIT);

        // Create the pipeline layout used by the compute shader
        const std::array<VkPushConstantRange, 1> pushRanges = {
            {{.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(shaderio::PushConstantCompute)}}
        };

        const std::array<VkDescriptorSetLayout, 2> computeDescriptorSetLayouts =
        {
            m_textureDescriptorSetLayout,
            m_commonDescriptorSetLayout // <-- This is your new, shared layout
        };

        // The pipeline layout is used to pass data to the pipeline, anything with "layout" in the shader
        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = uint32_t(computeDescriptorSetLayouts.size()),
            .pSetLayouts = computeDescriptorSetLayouts.data(),
            .pushConstantRangeCount = uint32_t(pushRanges.size()),
            .pPushConstantRanges = pushRanges.data(),
        };
        VK_CHECK(vkCreatePipelineLayout(m_context.getDevice(), &pipelineLayoutInfo, nullptr, &m_computePipelineLayout));
        DBG_VK_NAME(m_computePipelineLayout);

        // Creating the pipeline to run the compute shader
        const std::array<VkComputePipelineCreateInfo, 1> pipelineInfo{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                    .stage =
                    {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                        .module = compute,
                        .pName = computeEntryName.c_str(),
                    },
                    .layout = m_computePipelineLayout,
                }
            }
        };
        VK_CHECK(vkCreateComputePipelines(m_context.getDevice(), {}, uint32_t(pipelineInfo.size()), pipelineInfo.data(),
            nullptr, &m_computePipeline));
        DBG_VK_NAME(m_computePipeline);

        m_currentComputePipelineLayout = m_computePipelineLayout;
        
        m_computePipelines.push_back(m_computePipeline);
        
        m_computePipelinesLayouts.push_back(m_computePipelineLayout);

        return Wrap(m_computePipeline);
    }

    // Helper to download color attachment 1 (entity ID buffer) to CPU and keep pointer

    int32_t* VulkanRendererAPI::downloadColorAttachmentEntityID()
    {
        // Unmap and cleanup previous buffer if needed
        unmapDownloadedData();

        VkDevice device = m_context.getDevice();
        VkQueue queue = m_context.getGraphicsQueue().queue;
        VkCommandPool cmdPool = m_transientCmdPool;

        VkImage image = m_gBuffer.getColorImage(1);
        VkExtent2D size = m_gBuffer.getSize();
        size_t numPixels = size.width * size.height;
        VkDeviceSize bufferSize = numPixels * sizeof(int32_t);

        m_downloadedEntityIDBuffer = m_allocator.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_TO_CPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        );

        VkCommandBuffer cmd = utils::beginSingleTimeCommands(device, cmdPool);

        utils::cmdTransitionImageLayout(
            cmd,
            image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {size.width, size.height, 1};

        vkCmdCopyImageToBuffer(
            cmd,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_downloadedEntityIDBuffer.buffer,
            1,
            &copyRegion
        );

        utils::cmdTransitionImageLayout(
            cmd,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL
        );

        utils::endSingleTimeCommands(cmd, device, cmdPool, queue);

        vmaMapMemory(m_allocator, m_downloadedEntityIDBuffer.allocation, reinterpret_cast<void**>(&m_downloadedEntityIDPtr));

        auto id = reinterpret_cast<int32_t*>(m_downloadedEntityIDPtr);
    
        return id;
    }

    void VulkanRendererAPI::unmapDownloadedData()
    {
        if (m_downloadedEntityIDPtr)
        {
            vmaUnmapMemory(m_allocator, m_downloadedEntityIDBuffer.allocation);
            m_allocator.destroyBuffer(m_downloadedEntityIDBuffer);
            m_downloadedEntityIDPtr = nullptr;
            m_downloadedEntityIDBuffer = {};
        }
    }

    VanKComputePass* VulkanRendererAPI::BeginComputePass(VanKCommandBuffer cmd, VertexBuffer* buffer)
    {
        auto* result = new VanKComputePass
        {
            .VanKCommandBuffer = cmd,
            .VanKVertexBuffer = buffer
        };

        if (buffer != nullptr)
        {
            // Add a barrier to make sure nothing was writing to it, before updating its content
            utils::cmdBufferMemoryBarrier(Unwrap(cmd), static_cast<VkBuffer>(buffer->GetNativeHandle()),
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        }
        
        return result;
    }

    void VulkanRendererAPI::EndComputePass(VanKComputePass* computePass)
    {
        if (computePass->VanKVertexBuffer != nullptr)
        {
            // Add barrier to make sure the compute shader is finished before the vertex buffer is used
            utils::cmdBufferMemoryBarrier(Unwrap(computePass->VanKCommandBuffer), static_cast<VkBuffer>(computePass->VanKVertexBuffer->GetNativeHandle()), VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT);
        }

        delete computePass;
    }
}
