/**
 * @file   HostBuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general host accessible Vulkan buffer.
 */

#include "HostBuffer.h"
#include "DeviceBuffer.h"

namespace vku { namespace gfx {

    HostBuffer::HostBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Buffer{ device, usage, memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, queueFamilyIndices }
    {
    }

    HostBuffer::~HostBuffer() = default;

    HostBuffer::HostBuffer(const HostBuffer& rhs) :
        Buffer{ rhs.CopyWithoutData() }
    {
        std::vector<std::uint8_t> tmp(rhs.GetSize());
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

    void HostBuffer::InitializeData(std::size_t bufferSize, std::size_t dataSize, const void* data)
    {
        InitializeBuffer(bufferSize);
        UploadData(0, dataSize, data);
    }

    void HostBuffer::InitializeData(std::size_t size, const void* data)
    {
        InitializeData(size, size, data);
    }

    void HostBuffer::UploadData(std::size_t offset, std::size_t size, const void* data)
    {
        if (offset + size > GetSize()) {
            std::vector<std::uint8_t> tmp(offset);
            DownloadData(tmp);
            InitializeBuffer(offset + size);
            UploadDataInternal(0, offset, tmp.data());
        }

        UploadDataInternal(offset, size, data);
    }

    void HostBuffer::DownloadData(std::size_t size, void* data) const
    {
        GetDeviceMemory().CopyFromHostMemory(0, size, data);
    }

    void HostBuffer::UploadDataInternal(std::size_t offset, std::size_t size, const void* data) const
    {
        GetDeviceMemory().CopyToHostMemory(offset, size, data);
    }
}}
