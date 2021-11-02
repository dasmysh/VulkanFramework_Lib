/**
 * @file   Buffer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Declaration of a general Vulkan buffer.
 */

#pragma once

#include "main.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/ResourceViews.h"
#include "gfx/vk/wrappers/MemoryBoundResource.h"

namespace vkfw_core::gfx {

    class Queue;

    class Buffer : public VulkanObjectPrivateWrapper<vk::UniqueBuffer>, public MemoryBoundResource
    {
    public:
        Buffer(const LogicalDevice* device, std::string_view name, const vk::BufferUsageFlags& usage,
               const vk::MemoryPropertyFlags& memoryFlags = vk::MemoryPropertyFlags(),
               std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{});
        virtual ~Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void InitializeBuffer(std::size_t size, bool initMemory = true);

        void CopyBufferAsync(std::size_t srcOffset, Buffer& dstBuffer, std::size_t dstOffset, std::size_t size,
                             CommandBuffer& cmdBuffer);
        [[nodiscard]] CommandBuffer
        CopyBufferAsync(std::size_t srcOffset, Buffer& dstBuffer, std::size_t dstOffset, std::size_t size,
                        const Queue& queue, std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                        std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{},
                        std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence = {});
        [[nodiscard]] CommandBuffer
        CopyBufferAsync(Buffer& dstBuffer, const Queue& queue,
                        std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                        std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{},
                        std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence = {});
        void CopyBufferSync(Buffer& dstBuffer, const Queue& copyQueue);

        void AccessBarrier(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                           PipelineBarrier& barrier);
        [[nodiscard]] vk::Buffer GetBuffer(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                           PipelineBarrier& barrier);
        [[nodiscard]] const vk::Buffer*
        GetBufferPtr(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages, PipelineBarrier& barrier);
        // [[nodiscard]] vk::DeviceOrHostAddressConstKHR GetDeviceAddressConstNoBarrier() const;
        [[nodiscard]] vk::DeviceOrHostAddressConstKHR GetDeviceAddressConst(vk::AccessFlags2KHR access,
                                                                            vk::PipelineStageFlags2KHR pipelineStages,
                                                                            PipelineBarrier& barrier);
        [[nodiscard]] vk::DeviceOrHostAddressKHR GetDeviceAddress(vk::AccessFlags2KHR access,
                                                                  vk::PipelineStageFlags2KHR pipelineStages,
                                                                  PipelineBarrier& barrier);

        [[nodiscard]] std::size_t GetSize() const { return m_size; }
        [[nodiscard]] const DeviceMemory& GetDeviceMemory() const { return m_bufferDeviceMemory; }
        [[nodiscard]] vk::MemoryRequirements GetMemoryRequirements() const;
        [[nodiscard]] bool IsShaderDeviceAddress() const
        {
            return (m_usage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
                   == vk::BufferUsageFlagBits::eShaderDeviceAddress;
        }
        void BindMemory(vk::DeviceMemory deviceMemory, std::size_t offset);

    protected:
        [[nodiscard]] Buffer CopyWithoutData(std::string_view name) const
        {
            return Buffer{m_device, name, m_usage, m_bufferDeviceMemory.GetMemoryProperties(), m_queueFamilyIndices};
        }

    private:
        /** Holds the Vulkan device memory for the buffer. */
        DeviceMemory m_bufferDeviceMemory;
        /** Holds the current size of the buffer in bytes. */
        std::size_t m_size;
        /** Holds the buffer usage. */
        vk::BufferUsageFlags m_usage;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> m_queueFamilyIndices;
    };
}
