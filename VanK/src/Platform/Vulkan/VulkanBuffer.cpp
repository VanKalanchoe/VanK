#include "VulkanBuffer.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <span>
#include <vulkan/vulkan_core.h>

#include "VanK/Renderer/Buffer.h"
#include "VanK/Renderer/RendererAPI.h"
#include "vk_mem_alloc.h"
#include "VulkanRendererAPI.h"
#include "glm/fwd.hpp"
#include "../../../VanK-Editor/assets/shaders/shader_io.h"
namespace VanK
{
    VulkanVanKBuffer::VulkanVanKBuffer(uint64_t bufferSize){}
    VulkanVanKBuffer::~VulkanVanKBuffer(){}
    void VulkanVanKBuffer::Bind() const{}
    void VulkanVanKBuffer::Unbind() const{}

    // VulkanVertexBuffer constructor
    VulkanVertexBuffer::VulkanVertexBuffer(uint64_t bufferSize)
    {
        std::cout << "Creating Empty Vertex Buffer" << std::endl;

        VulkanRendererAPI& instance = VulkanRendererAPI::Get();

        // Create an empty buffer (no initial data)
        utils::Buffer vertexBuffer = instance.GetAllocator().createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY 
        );
        DBG_VK_NAME(vertexBuffer.buffer);

        m_vertexBuffer = vertexBuffer;
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        instance.GetAllocator().destroyBuffer(m_vertexBuffer);
    }

    void VulkanVertexBuffer::Bind() const
    {
    }

    void VulkanVertexBuffer::Unbind() const
    {
    }

    // VertexBuffer Upload and Update
    void VulkanVertexBuffer::Upload(const void* data, size_t size)
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
        VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(), instance.GetTransientCmdPool());
    
        std::span<const uint8_t> dataSpan(static_cast<const uint8_t*>(data), size);
        utils::Buffer stagingBuffer = instance.GetAllocator().createStagingBuffer(dataSpan);
    
        const std::array<VkBufferCopy, 1> copyRegion{{{.size = size}}};
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_vertexBuffer.buffer, uint32_t(copyRegion.size()), copyRegion.data());
    
        utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(), instance.GetTransientCmdPool(), instance.GetContext().getGraphicsQueue().queue);
        instance.GetAllocator().freeStagingBuffers();
    }

    // VulkanIndexBuffer constructor
    VulkanIndexBuffer::VulkanIndexBuffer(uint64_t bufferSize)
        : m_Count(bufferSize / sizeof(uint32_t))  // Calculate count from size
    {
        std::cout << "Creating Empty Index Buffer" << std::endl;

        VulkanRendererAPI& instance = VulkanRendererAPI::Get();

        // Create an empty buffer (no initial data)
        utils::Buffer indexBuffer = instance.GetAllocator().createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        DBG_VK_NAME(indexBuffer.buffer);

        m_indexBuffer = indexBuffer;
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
            VulkanRendererAPI& instance = VulkanRendererAPI::Get();
            instance.GetAllocator().destroyBuffer(m_indexBuffer);
    }

    void VulkanIndexBuffer::Bind() const
    {
    }

    void VulkanIndexBuffer::Unbind() const
    {
    }

    // IndexBuffer Upload and Update
    void VulkanIndexBuffer::Upload(const void* data, size_t size)
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
        VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(), instance.GetTransientCmdPool());
    
        std::span<const uint8_t> dataSpan(static_cast<const uint8_t*>(data), size);
        utils::Buffer stagingBuffer = instance.GetAllocator().createStagingBuffer(dataSpan);
    
        const std::array<VkBufferCopy, 1> copyRegion{{{.size = size}}};
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_indexBuffer.buffer, uint32_t(copyRegion.size()), copyRegion.data());
    
        utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(), instance.GetTransientCmdPool(), instance.GetContext().getGraphicsQueue().queue);
        instance.GetAllocator().freeStagingBuffers();
    }
    
    /*-- Create a buffer -*/
    /* 
     * UBO: VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
     *        + VMA_MEMORY_USAGE_CPU_TO_GPU
     * SSBO: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
     *        + VMA_MEMORY_USAGE_CPU_TO_GPU // Use this if the CPU will frequently update the buffer
     *        + VMA_MEMORY_USAGE_GPU_ONLY // Use this if the CPU will rarely update the buffer
     *        + VMA_MEMORY_USAGE_GPU_TO_CPU  // Use this when you need to read back data from the SSBO to the CPU
     *      ----
     *        + VMA_ALLOCATION_CREATE_MAPPED_BIT // Automatically maps the buffer upon creation
     *        + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT // If the CPU will sequentially write to the buffer's memory,
     */
    VulkanTransferBuffer::VulkanTransferBuffer(uint64_t size, VanKTransferBufferUsage usage)
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();

        VmaMemoryUsage memoryUsage = {};
        
        switch (usage)
        {
            case VanKTransferBufferUsageUpload: memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
            case VanKTransferBufferUsageDownload: memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
        }

        // Create a staging buffer
        m_transferBuffer = instance.GetAllocator().createBuffer(size, VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
                                            memoryUsage,
                                            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    VulkanTransferBuffer::~VulkanTransferBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        /*instance.GetAllocator().freeStagingBuffers();*/
        instance.GetAllocator().destroyBuffer(m_transferBuffer);
    }

    void VulkanTransferBuffer::Bind() const
    {
    }

    void VulkanTransferBuffer::Unbind() const
    {
    }
    
    void* VulkanTransferBuffer::MapTransferBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        // Map and copy data to the staging buffer
        void* data;
        if (vmaMapMemory(instance.GetAllocator(), m_transferBuffer.allocation, &data) != VK_SUCCESS) {
            return nullptr;
        }
        return data;
    }

    void VulkanTransferBuffer::UnMapTransferBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        vmaUnmapMemory(instance.GetAllocator(), m_transferBuffer.allocation);
    }
        
    void VulkanTransferBuffer::UploadToGPUBuffer(VanKCommandBuffer cmd, VanKTransferBufferLocation location, VanKBufferRegion bufferRegion)
    {
        if (bufferRegion.size == 0)
            return; // or skip the copy safely

        if (bufferRegion.buffer == VK_NULL_HANDLE) {
            std::cerr << "Error: bufferRegion.buffer is null!" << std::endl;
            return;
        }
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = location.offset;
        copyRegion.dstOffset = bufferRegion.offset;
        copyRegion.size = bufferRegion.size;
        //maybe copyregion array idk

        // Add a barrier to make sure nothing was writing to it, before updating its content
        utils::cmdBufferMemoryBarrier(Unwrap(cmd), static_cast<VkBuffer>(bufferRegion.buffer->GetNativeHandle()),
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        
        vkCmdCopyBuffer(Unwrap(cmd), m_transferBuffer.buffer, static_cast<VkBuffer>(bufferRegion.buffer->GetNativeHandle()), 1, &copyRegion);
        
        // Add barrier to make sure the buffer is updated before the fragment shader uses it
        utils::cmdBufferMemoryBarrier(Unwrap(cmd), static_cast<VkBuffer>(bufferRegion.buffer->GetNativeHandle()),
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }

    // VulkanUniformBuffer constructor
    VulkanUniformBuffer::VulkanUniformBuffer(uint64_t size)
    {
        std::cout << "Creating Uniform Buffer" << std::endl;

        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        // Create a buffer (UBO) to store the scene information
        m_uniformBuffer = instance.GetAllocator().createBuffer(size,
                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                     VMA_MEMORY_USAGE_GPU_ONLY);
        DBG_VK_NAME(m_uniformBuffer.buffer);
    }

    VulkanUniformBuffer::~VulkanUniformBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        /*instance.GetAllocator().freeStagingBuffers();*/
        instance.GetAllocator().destroyBuffer(m_uniformBuffer);
    }

    void VulkanUniformBuffer::Bind() const
    {
    
    }

    void VulkanUniformBuffer::Unbind() const
    {
    
    }

    // UniformBuffer Update (special case with memory barriers)
    void VulkanUniformBuffer::Update(VanKCommandBuffer cmd, const void* data, size_t size)
    {
        // Add a memory barrier before updating (optional, but good practice)
        utils::cmdBufferMemoryBarrier(
            Unwrap(cmd),
            m_uniformBuffer.buffer,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT
        );

        // Update the buffer
        vkCmdUpdateBuffer(Unwrap(cmd), m_uniformBuffer.buffer, 0, size, data);

        // Add a memory barrier after updating
        utils::cmdBufferMemoryBarrier(
            Unwrap(cmd),
            m_uniformBuffer.buffer,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
        );
    }

    // VulkanStorageBuffer constructor
    VulkanStorageBuffer::VulkanStorageBuffer(uint64_t bufferSize)
    {
        std::cout << "Creating Empty Storage Buffer" << std::endl;
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
        m_storageBuffer = instance.GetAllocator().createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        DBG_VK_NAME(m_storageBuffer.buffer);
    }

    VulkanStorageBuffer::~VulkanStorageBuffer()
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        /*instance.GetAllocator().freeStagingBuffers();*/
        instance.GetAllocator().destroyBuffer(m_storageBuffer);
    }

    void VulkanStorageBuffer::Bind() const
    {
    }

    void VulkanStorageBuffer::Unbind() const
    {
    }

    // StorageBuffer Upload and Update
    void VulkanStorageBuffer::Upload(const void* data, size_t size)
    {
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
        VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(), instance.GetTransientCmdPool());
    
        std::span<const uint8_t> dataSpan(static_cast<const uint8_t*>(data), size);
        utils::Buffer stagingBuffer = instance.GetAllocator().createStagingBuffer(dataSpan);
    
        const std::array<VkBufferCopy, 1> copyRegion{{{.size = size}}};
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_storageBuffer.buffer, uint32_t(copyRegion.size()), copyRegion.data());
    
        utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(), instance.GetTransientCmdPool(), instance.GetContext().getGraphicsQueue().queue);
        instance.GetAllocator().freeStagingBuffers();
    }
}