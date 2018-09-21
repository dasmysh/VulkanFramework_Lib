/**
 * @file   BufferGroup.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.27
 *
 * @brief  Declaration of a class managing multiple Vulkan buffers for simultaneous memory usage.
 */

#pragma once

#include "main.h"
#include "gfx/vk/LogicalDevice.h"
#include "core/type_traits.h"
#include "DeviceBuffer.h"

namespace vku::gfx {

    class HostBuffer;
    class QueuedDeviceTransfer;

    class [[deprecated("Replaced by MemoryGroup that also handles textures.")]] BufferGroup
    {
    public:
        explicit BufferGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags());
        virtual ~BufferGroup();
        BufferGroup(const BufferGroup&) = delete;
        BufferGroup& operator=(const BufferGroup&) = delete;
        BufferGroup(BufferGroup&&) noexcept;
        BufferGroup& operator=(BufferGroup&&) noexcept;

        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const void* data,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        void FinalizeGroup(QueuedDeviceTransfer* transfer = nullptr);

        DeviceBuffer* GetBuffer(unsigned int bufferIdx) { return &deviceBuffers_[bufferIdx]; }

        template<class T> std::enable_if_t<has_contiguous_memory<T>::value> AddBufferToGroup(vk::BufferUsageFlags usage, const T& data,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

    private:
        void FillAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const;

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan device memory for the device buffers. */
        vk::DeviceMemory deviceBufferMemory_;
        /** Holds the Vulkan device memory for the host buffers. */
        vk::DeviceMemory hostBufferMemory_;
        /** Holds the device buffers. */
        std::vector<DeviceBuffer> deviceBuffers_;
        /** Holds the host buffers. */
        std::vector<HostBuffer> hostBuffers_;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_;

        /** Holds the buffer contents that need to be transfered. */
        std::vector<std::pair<std::size_t, const void*>> bufferContents_;
    };

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value> BufferGroup::AddBufferToGroup(vk::BufferUsageFlags usage,
        const T& data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        AddBufferToGroup(usage, byteSizeOf(data), data.data(), queueFamilyIndices);
    }
}
