/**
 * @file   DeviceBuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general device memory Vulkan buffer.
 */

#include "DeviceBuffer.h"
#include "LogicalDevice.h"
#include "HostBuffer.h"

namespace vku { namespace gfx {

    DeviceBuffer::DeviceBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& queueFamilyIndices) :
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
}}
