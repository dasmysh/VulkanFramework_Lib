/**
 * @file   DeviceBuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general device memory Vulkan buffer.
 */

#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/buffers/HostBuffer.h"

namespace vku::gfx {

    DeviceBuffer::DeviceBuffer(const LogicalDevice* device, const vk::BufferUsageFlags& usage,
        const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Buffer{ device, usage, memoryFlags | vk::MemoryPropertyFlagBits::eDeviceLocal, queueFamilyIndices }
    {
    }

    DeviceBuffer::~DeviceBuffer() = default;

    DeviceBuffer::DeviceBuffer(DeviceBuffer&& rhs) noexcept :
    Buffer{ std::move(rhs) }
    {
    }

    DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& rhs) noexcept
    {
        this->~DeviceBuffer();
        Buffer::operator=(std::move(rhs));
        return *this;
    }
}
