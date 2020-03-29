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

namespace vkfw_core::gfx {

    class Buffer
    {
    public:
        Buffer(const LogicalDevice* device, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryFlags = vk::MemoryPropertyFlags(),
            std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{});
        virtual ~Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void InitializeBuffer(std::size_t size, bool initMemory = true);

        void CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset, std::size_t size,
                             vk::CommandBuffer cmdBuffer) const;
        [[nodiscard]] vk::UniqueCommandBuffer
        CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset, std::size_t size,
                        std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
                        const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
                        const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
                        vk::Fence fence = vk::Fence()) const;
        [[nodiscard]] vk::UniqueCommandBuffer
        CopyBufferAsync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
                        const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
                        const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
                        vk::Fence fence = vk::Fence()) const;
        void CopyBufferSync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const;

        [[nodiscard]] std::size_t GetSize() const { return size_; }
        [[nodiscard]] vk::Buffer GetBuffer() const { return *buffer_; }
        [[nodiscard]] const vk::Buffer* GetBufferPtr() const { return &(*buffer_); }
        [[nodiscard]] const DeviceMemory& GetDeviceMemory() const { return bufferDeviceMemory_; }

    protected:
        [[nodiscard]] Buffer CopyWithoutData() const
        {
            return Buffer{device_, usage_, bufferDeviceMemory_.GetMemoryProperties(), queueFamilyIndices_};
        }

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan buffer object. */
        vk::UniqueBuffer buffer_;
        /** Holds the Vulkan device memory for the buffer. */
        DeviceMemory bufferDeviceMemory_;
        /** Holds the current size of the buffer in bytes. */
        std::size_t size_;
        /** Holds the buffer usage. */
        vk::BufferUsageFlags usage_;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> queueFamilyIndices_;
    };
}
