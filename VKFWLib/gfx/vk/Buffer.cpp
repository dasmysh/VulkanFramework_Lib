/**
 * @file   Buffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general Vulkan buffer.
 */

#include "Buffer.h"
#include "LogicalDevice.h"

namespace vku { namespace gfx {

    Buffer::Buffer(LogicalDevice* device, vk::BufferUsageFlags usage) :
        device_{ device },
        size_{ 0 },
        usage_{ usage }
    {
    }

    Buffer::~Buffer()
    {
        if (buffer_) device_->GetDevice().destroyBuffer(buffer_);
        buffer_ = vk::Buffer();
        if (bufferDeviceMemory_) device_->GetDevice().freeMemory(bufferDeviceMemory_);
        bufferDeviceMemory_ = vk::DeviceMemory();
    }

    Buffer::Buffer(const Buffer& rhs) :
        device_{ rhs.device_ },
        size_{ rhs.size_ },
        usage_{ rhs.usage_ }
    {
        std::vector<int8_t> tmp(size_);
        rhs.DownloadData(tmp);
        InitializeData(tmp);
    }

    Buffer& Buffer::operator=(const Buffer& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    Buffer::Buffer(Buffer&& rhs) noexcept :
        device_{ rhs.device_ },
        buffer_{ rhs.buffer_ },
        bufferDeviceMemory_{ rhs.bufferDeviceMemory_ },
        size_{ rhs.size_ },
        usage_{ rhs.usage_ }
    {
        rhs.buffer_ = vk::Buffer();
        rhs.bufferDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        device_ = rhs.device_;
        buffer_ = rhs.buffer_;
        bufferDeviceMemory_ = rhs.bufferDeviceMemory_;
        size_ = rhs.size_;
        usage_ = rhs.usage_;
        rhs.buffer_ = vk::Buffer();
        rhs.bufferDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
        return *this;
    }

    void Buffer::InitializeData(size_t size, const void* data)
    {
        InitializeBuffer(size);
        UploadData(0, size, data);
    }

    void Buffer::UploadData(size_t offset, size_t size, const void* data)
    {
        if (offset + size > size_) {
            std::vector<int8_t> tmp(offset);
            DownloadData(tmp);
            InitializeBuffer(offset + size);
            UploadDataInternal(0, offset, tmp.data());
        }

        UploadDataInternal(offset, size, data);
    }

    void Buffer::DownloadData(size_t size, void* data) const
    {
        auto deviceMem = device_->GetDevice().mapMemory(bufferDeviceMemory_, 0, size, vk::MemoryMapFlags());
        memcpy(data, deviceMem, size);
        device_->GetDevice().unmapMemory(bufferDeviceMemory_);
    }

    uint32_t Buffer::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        auto memProperties = device_->GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Buffer::InitializeBuffer(size_t size)
    {
        this->~Buffer();

        size_ = size;
        vk::BufferCreateInfo bufferCreateInfo{ vk::BufferCreateFlags(), static_cast<vk::DeviceSize>(size_), usage_, vk::SharingMode::eExclusive };
        buffer_ = device_->GetDevice().createBuffer(bufferCreateInfo);

        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer_);
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
        bufferDeviceMemory_ = device_->GetDevice().allocateMemory(allocInfo);
        device_->GetDevice().bindBufferMemory(buffer_, bufferDeviceMemory_, 0);
    }

    void Buffer::UploadDataInternal(size_t offset, size_t size, const void* data) const
    {
        auto deviceMem = device_->GetDevice().mapMemory(bufferDeviceMemory_, offset, size, vk::MemoryMapFlags());
        memcpy(deviceMem, data, size);
        device_->GetDevice().unmapMemory(bufferDeviceMemory_);
    }
}}
