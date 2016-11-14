/**
 * @file   Buffer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Declaration of a general Vulkan buffer.
 */

#pragma once

#include "main.h"
#include "LogicalDevice.h"

namespace vku { namespace gfx {

    class Buffer
    {
    public:
        Buffer(const LogicalDevice* device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        virtual ~Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void InitializeBuffer(size_t size);
        vk::CommandBuffer CopyBufferAsync(const Buffer& dstBuffer, std::pair<uint32_t, uint32_t> copyQueueIdx,
                const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
                const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
                vk::Fence fence = vk::Fence()) const;
        void CopyBufferSync(const Buffer& dstBuffer, std::pair<uint32_t, uint32_t> copyQueueIdx) const;

        size_t GetSize() const { return size_; }
        const vk::Buffer* GetBuffer() const { return &buffer_; }

    protected:
        Buffer CopyWithoutData() const { return Buffer{ device_, usage_, memoryProperties_, queueFamilyIndices_ }; }
        vk::DeviceMemory GetDeviceMemory() const { return bufferDeviceMemory_; }
        vk::Device GetDevice() const { return device_->GetDevice(); }

    private:
        uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan buffer object. */
        vk::Buffer buffer_;
        /** Holds the Vulkan device memory for the buffer. */
        vk::DeviceMemory bufferDeviceMemory_;
        /** Holds the current size of the buffer in bytes. */
        size_t size_;
        /** Holds the buffer usage. */
        vk::BufferUsageFlags usage_;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_;
        /** Holds the queue family indices. */
        std::vector<uint32_t> queueFamilyIndices_;
    };
}}
