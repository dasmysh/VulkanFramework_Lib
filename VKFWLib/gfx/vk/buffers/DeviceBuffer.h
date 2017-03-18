/**
 * @file   DeviceBuffer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Declaration of a general device memory Vulkan buffer.
 */

#pragma once

#include "main.h"
#include "Buffer.h"

namespace vku::gfx {
    class HostBuffer;

    class DeviceBuffer final : public Buffer
    {
    public:
        DeviceBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ~DeviceBuffer();
        DeviceBuffer(const DeviceBuffer&) = delete;
        DeviceBuffer& operator=(const DeviceBuffer&) = delete;
        DeviceBuffer(DeviceBuffer&&) noexcept;
        DeviceBuffer& operator=(DeviceBuffer&&) noexcept;
    };
}
