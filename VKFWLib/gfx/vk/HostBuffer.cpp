/**
 * @file   HostBuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general host accessible Vulkan buffer.
 */

#include "HostBuffer.h"
#include "LogicalDevice.h"
#include "DeviceBuffer.h"

namespace vku { namespace gfx {

    HostBuffer::HostBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& queueFamilyIndices) :
        Buffer{ device, usage, memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, queueFamilyIndices }
    {
    }

    HostBuffer::~HostBuffer() = default;

    HostBuffer::HostBuffer(const HostBuffer& rhs) :
        Buffer{ rhs.CopyWithoutData() }
    {
        std::vector<int8_t> tmp(GetSize());
        rhs.DownloadData(tmp);
        InitializeData(tmp);
    }

    HostBuffer& HostBuffer::operator=(const HostBuffer& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    HostBuffer::HostBuffer(HostBuffer&& rhs) noexcept :
        Buffer{ std::move(rhs) }
    {
    }

    HostBuffer& HostBuffer::operator=(HostBuffer&& rhs) noexcept
    {
        this->~HostBuffer();
        Buffer::operator=(std::move(rhs));
        return *this;
    }

    void HostBuffer::InitializeData(size_t size, const void* data)
    {
        InitializeBuffer(size);
        UploadData(0, size, data);
    }

    void HostBuffer::UploadData(size_t offset, size_t size, const void* data)
    {
        if (offset + size > GetSize()) {
            std::vector<int8_t> tmp(offset);
            DownloadData(tmp);
            InitializeBuffer(offset + size);
            UploadDataInternal(0, offset, tmp.data());
        }

        UploadDataInternal(offset, size, data);
    }

    void HostBuffer::DownloadData(size_t size, void* data) const
    {
        auto deviceMem = GetDevice().mapMemory(GetDeviceMemory(), 0, size, vk::MemoryMapFlags());
        memcpy(data, deviceMem, size);
        GetDevice().unmapMemory(GetDeviceMemory());
    }

    void HostBuffer::UploadDataInternal(size_t offset, size_t size, const void* data) const
    {
        auto deviceMem = GetDevice().mapMemory(GetDeviceMemory(), offset, size, vk::MemoryMapFlags());
        memcpy(deviceMem, data, size);
        GetDevice().unmapMemory(GetDeviceMemory());
    }
}}
