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

namespace vku { namespace gfx {

    class Buffer
    {
    public:
        Buffer(const LogicalDevice* device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        virtual ~Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void InitializeBuffer(std::size_t size, bool initMemory = true);
        vk::CommandBuffer CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset,
            std::size_t size, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence()) const;
        vk::CommandBuffer CopyBufferAsync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence()) const;
        void CopyBufferSync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const;

        std::size_t GetSize() const { return size_; }
        vk::Buffer GetBuffer() const { return buffer_; }
        const vk::Buffer* GetBufferPtr() const { return &buffer_; }
        const DeviceMemory& GetDeviceMemory() const { return bufferDeviceMemory_; }

    protected:
        Buffer CopyWithoutData() const { return Buffer{ device_, usage_, bufferDeviceMemory_.GetMemoryProperties(), queueFamilyIndices_ }; }

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan buffer object. */
        vk::Buffer buffer_;
        /** Holds the Vulkan device memory for the buffer. */
        DeviceMemory bufferDeviceMemory_;
        /** Holds the current size of the buffer in bytes. */
        std::size_t size_;
        /** Holds the buffer usage. */
        vk::BufferUsageFlags usage_;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> queueFamilyIndices_;
    };
}}
