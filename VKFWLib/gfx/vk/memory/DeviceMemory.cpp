/**
 * @file   DeviceMemory.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of vulkan device memory objects.
 */

#include "DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/buffers/Buffer.h"

namespace vku { namespace gfx {

    DeviceMemory::DeviceMemory(const LogicalDevice* device, vk::MemoryPropertyFlags properties) :
        device_{ device },
        size_{ 0 },
        memoryProperties_{ properties }
    {
    }

    DeviceMemory::DeviceMemory(const LogicalDevice* device, vk::MemoryRequirements memRequirements, vk::MemoryPropertyFlags properties) :
        DeviceMemory(device, properties)
    {
        InitializeMemory(memRequirements);
    }

    DeviceMemory::DeviceMemory(DeviceMemory&& rhs) noexcept :
        device_{ rhs.device_ },
        vkDeviceMemory_{ rhs.vkDeviceMemory_},
        size_{ rhs.size_ },
        memoryProperties_{ rhs.memoryProperties_ }
    {
        rhs.vkDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
    }

    DeviceMemory& DeviceMemory::operator=(DeviceMemory&& rhs) noexcept
    {
        this->~DeviceMemory();
        device_ = rhs.device_;
        vkDeviceMemory_ = rhs.vkDeviceMemory_;
        size_ = rhs.size_;
        memoryProperties_ = rhs.memoryProperties_;
        rhs.vkDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
        return *this;
    }

    DeviceMemory::~DeviceMemory()
    {
        if (vkDeviceMemory_) device_->GetDevice().freeMemory(vkDeviceMemory_);
        vkDeviceMemory_ = vk::DeviceMemory();
    }

    void DeviceMemory::InitializeMemory(const vk::MemoryRequirements& memRequirements)
    {
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size,
            FindMemoryType(device_, memRequirements.memoryTypeBits, memoryProperties_) };
        InitializeMemory(allocInfo);
    }

    void DeviceMemory::InitializeMemory(const vk::MemoryAllocateInfo& memAllocateInfo)
    {
        vkDeviceMemory_ = device_->GetDevice().allocateMemory(memAllocateInfo);
    }

    void DeviceMemory::BindToBuffer(Buffer& buffer, size_t offset) const
    {
        device_->GetDevice().bindBufferMemory(buffer.GetBuffer(), vkDeviceMemory_, offset);
    }

    void DeviceMemory::BindToTexture(Texture& texture, size_t offset) const
    {
        device_->GetDevice().bindImageMemory(texture.GetImage(), vkDeviceMemory_, offset);
    }

    void DeviceMemory::CopyToHostMemory(size_t offset, size_t size, const void* data) const
    {
        MapAndProcess(offset, size, [data](void* deviceMem, size_t size) { memcpy(deviceMem, data, size); });
    }

    void DeviceMemory::CopyToHostMemory(size_t offset, const vk::SubresourceLayout& layout, const glm::u64vec4& dataSize, const void* data) const
    {
        auto dataBytes = reinterpret_cast<const uint8_t*>(data);
        MapAndProcess(offset, layout, dataSize, [dataBytes](void* deviceMem, size_t offset, size_t size) { memcpy(deviceMem, &dataBytes[offset], size); });
    }

    void DeviceMemory::CopyFromHostMemory(size_t offset, size_t size, void* data) const
    {
        MapAndProcess(offset, size, [data](void* deviceMem, size_t size) { memcpy(data, deviceMem, size); });
    }

    void DeviceMemory::CopyFromHostMemory(size_t offset, const vk::SubresourceLayout& layout, const glm::u64vec4& dataSize, void* data) const
    {
        auto dataBytes = reinterpret_cast<uint8_t*>(data);
        MapAndProcess(offset, layout, dataSize, [dataBytes](void* deviceMem, size_t offset, size_t size) { memcpy(&dataBytes[offset], deviceMem, size); });
    }

    uint32_t DeviceMemory::FindMemoryType(const LogicalDevice* device, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (CheckMemoryType(memProperties, i, typeFilter, properties)) return i;
        }

        LOG(FATAL) << "Failed to find suitable memory type (" << vk::to_string(properties) << ").";
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    bool DeviceMemory::CheckMemoryType(const LogicalDevice* device, uint32_t typeToCheck, uint32_t typeFilter,
        vk::MemoryPropertyFlags properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        return CheckMemoryType(memProperties, typeToCheck, typeFilter, properties);
    }

    bool DeviceMemory::CheckMemoryType(const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeToCheck,
        uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        if ((typeFilter & (1 << typeToCheck)) && (memProperties.memoryTypes[typeToCheck].propertyFlags & properties) == properties) return true;
        return false;
    }

    void DeviceMemory::MapAndProcess(size_t offset, size_t size, const std::function<void (void* deviceMem, size_t size)>& processFunc) const
    {
        assert(memoryProperties_ & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        auto deviceMem = device_->GetDevice().mapMemory(vkDeviceMemory_, offset, size, vk::MemoryMapFlags());
        processFunc(deviceMem, size);
        device_->GetDevice().unmapMemory(vkDeviceMemory_);
    }

    void DeviceMemory::MapAndProcess(size_t offset, const vk::SubresourceLayout& layout, const glm::u64vec4& dataSize,
        const std::function<void(void* deviceMem, size_t offset, size_t size)>& processFunc) const
    {
        assert(memoryProperties_ & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        auto deviceMem = device_->GetDevice().mapMemory(vkDeviceMemory_, offset + layout.offset, layout.size);
        auto deviceBytes = reinterpret_cast<uint8_t*>(deviceMem);

        if (layout.rowPitch == dataSize.x && layout.depthPitch == dataSize.y && layout.arrayPitch == dataSize.z) {
            processFunc(deviceBytes, 0, dataSize.x * dataSize.y * dataSize.z * dataSize.w);
        }
        else if (layout.rowPitch == dataSize.x && layout.depthPitch == dataSize.y) {
            auto sliceSize = dataSize.x * dataSize.y * dataSize.z;
            for (auto slice = 0U; slice < dataSize.w; ++slice)
                processFunc(&deviceBytes[slice * layout.arrayPitch], slice * sliceSize, sliceSize);
        }
        else if (layout.rowPitch == dataSize.x) {
            for (auto slice = 0U; slice < dataSize.w; ++slice) {
                for (auto z = 0U; z < dataSize.z; ++z) {
                    auto deviceMemPos = slice * layout.arrayPitch + z * layout.depthPitch;
                    auto dataMemPos = (slice * dataSize.z + z) * dataSize.x * dataSize.y;
                    processFunc(&deviceBytes[deviceMemPos], dataMemPos, dataSize.x * dataSize.y);
                }
            }
        }
        else {
            for (auto slice = 0U; slice < dataSize.w; ++slice) {
                for (auto z = 0U; z < dataSize.z; ++z) {
                    for (auto y = 0U; y < dataSize.y; ++y) {
                        auto deviceMemPos = slice * layout.arrayPitch + z * layout.depthPitch + y * layout.rowPitch;
                        auto dataMemPos = ((slice * dataSize.z + z) * dataSize.x + y) * dataSize.y;
                        processFunc(&deviceBytes[deviceMemPos], dataMemPos, dataSize.x);
                    }
                }
            }
        }

        device_->GetDevice().unmapMemory(vkDeviceMemory_);
    }
}}
