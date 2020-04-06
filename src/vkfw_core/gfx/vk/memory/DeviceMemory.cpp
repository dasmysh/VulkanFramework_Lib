/**
 * @file   DeviceMemory.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of vulkan device memory objects.
 */

#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/buffers/Buffer.h"

namespace vkfw_core::gfx {

    DeviceMemory::DeviceMemory(const LogicalDevice* device, const vk::MemoryPropertyFlags& properties) :
        device_{ device },
        size_{ 0 },
        memoryProperties_{ properties }
    {
    }

    DeviceMemory::DeviceMemory(const LogicalDevice* device, vk::MemoryRequirements memRequirements,
                               const vk::MemoryPropertyFlags& properties)
        :
        DeviceMemory(device, properties)
    {
        InitializeMemory(memRequirements);
    }

    DeviceMemory::DeviceMemory(DeviceMemory&& rhs) noexcept :
        device_{ rhs.device_ },
        vkDeviceMemory_{ std::move(rhs.vkDeviceMemory_) },
        size_{ rhs.size_ },
        memoryProperties_{ rhs.memoryProperties_ }
    {
        rhs.size_ = 0;
    }

    DeviceMemory& DeviceMemory::operator=(DeviceMemory&& rhs) noexcept
    {
        this->~DeviceMemory();
        device_ = rhs.device_;
        vkDeviceMemory_ = std::move(rhs.vkDeviceMemory_);
        size_ = rhs.size_;
        memoryProperties_ = rhs.memoryProperties_;
        rhs.size_ = 0;
        return *this;
    }

    DeviceMemory::~DeviceMemory() = default;

    void DeviceMemory::InitializeMemory(const vk::MemoryRequirements& memRequirements)
    {
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size,
            FindMemoryType(device_, memRequirements.memoryTypeBits, memoryProperties_) };
        InitializeMemory(allocInfo);
    }

    void DeviceMemory::InitializeMemory(const vk::MemoryAllocateInfo& memAllocateInfo)
    {
        vkDeviceMemory_ = device_->GetDevice().allocateMemoryUnique(memAllocateInfo);
    }

    void DeviceMemory::BindToBuffer(Buffer& buffer, std::size_t offset) const
    {
        device_->GetDevice().bindBufferMemory(buffer.GetBuffer(), *vkDeviceMemory_, offset);
    }

    void DeviceMemory::BindToTexture(Texture& texture, std::size_t offset) const
    {
        device_->GetDevice().bindImageMemory(texture.GetImage(), *vkDeviceMemory_, offset);
    }

    void DeviceMemory::CopyToHostMemory(std::size_t offset, std::size_t size, const void* data) const
    {
        assert(vkDeviceMemory_ && "Device memory must be valid.");
        MapAndProcess(offset, size, [data](void* deviceMem, std::size_t size) { memcpy(deviceMem, data, size); });
    }

    void DeviceMemory::CopyToHostMemory(std::size_t offsetToTexture, const glm::u32vec3& offset,
        const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, const void* data) const
    {
        assert(vkDeviceMemory_ && "Device memory must be valid.");
        auto dataBytes = reinterpret_cast<const std::uint8_t*>(data); // NOLINT
        MapAndProcess(offsetToTexture, offset, layout, dataSize,
                      [dataBytes](void* deviceMem, std::size_t offset, std::size_t size) {
                          memcpy(deviceMem, &dataBytes[offset], size); // NOLINT
                      });
    }

    void DeviceMemory::CopyFromHostMemory(std::size_t offset, std::size_t size, void* data) const
    {
        assert(vkDeviceMemory_ && "Device memory must be valid.");
        MapAndProcess(offset, size, [data](void* deviceMem, std::size_t size) { memcpy(data, deviceMem, size); });
    }

    void DeviceMemory::CopyFromHostMemory(std::size_t offsetToTexture, const glm::u32vec3& offset,
        const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, void* data) const
    {
        assert(vkDeviceMemory_ && "Device memory must be valid.");
        auto dataBytes = reinterpret_cast<std::uint8_t*>(data); // NOLINT
        MapAndProcess(offsetToTexture, offset, layout, dataSize,
                      [dataBytes](void* deviceMem, std::size_t offset, std::size_t size) {
                          memcpy(&dataBytes[offset], deviceMem, size); // NOLINT
                      });
    }

    std::uint32_t DeviceMemory::FindMemoryType(const LogicalDevice* device, std::uint32_t typeFilter,
                                               const vk::MemoryPropertyFlags& properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (CheckMemoryType(memProperties, i, typeFilter, properties)) { return i; }
        }

        spdlog::critical("Failed to find suitable memory type ({}).", vk::to_string(properties));
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    bool DeviceMemory::CheckMemoryType(const LogicalDevice* device, std::uint32_t typeToCheck, std::uint32_t typeFilter,
                                       const vk::MemoryPropertyFlags& properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        return CheckMemoryType(memProperties, typeToCheck, typeFilter, properties);
    }

    bool DeviceMemory::CheckMemoryType(const vk::PhysicalDeviceMemoryProperties& memProperties, std::uint32_t typeToCheck,
        std::uint32_t typeFilter, const vk::MemoryPropertyFlags& properties)
    {
        return (((typeFilter & (1U << typeToCheck)) != 0U)
                && (memProperties.memoryTypes[typeToCheck].propertyFlags & properties) == properties); // NOLINT
    }

    void DeviceMemory::MapAndProcess(std::size_t offset, std::size_t size,
                                     const function_view<void(void* deviceMem, std::size_t size)>& processFunc) const
    {
        assert(memoryProperties_ & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        auto deviceMem = device_->GetDevice().mapMemory(*vkDeviceMemory_, offset, size, vk::MemoryMapFlags());
        processFunc(deviceMem, size);
        device_->GetDevice().unmapMemory(*vkDeviceMemory_);
    }

    void DeviceMemory::MapAndProcess(std::size_t offsetToTexture, const glm::u32vec3& offset,
        const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize,
        const function_view<void(void* deviceMem, std::size_t offset, std::size_t size)>& processFunc) const
    {
        assert(memoryProperties_ & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        auto mapOffset = offset.z * layout.depthPitch + offsetToTexture;
        auto deviceMem = device_->GetDevice().mapMemory(*vkDeviceMemory_, mapOffset + layout.offset, layout.size);
        auto deviceBytes = reinterpret_cast<std::uint8_t*>(deviceMem); // NOLINT

        if (layout.rowPitch == dataSize.x && layout.depthPitch == dataSize.y
            && offset.x == 0 && offset.y == 0) {
            processFunc(deviceBytes, 0, static_cast<std::size_t>(dataSize.x) * static_cast<std::size_t>(dataSize.y) * static_cast<std::size_t>(dataSize.z));
        }
        else if (layout.rowPitch == dataSize.x && offset.x == 0) {
            for (auto z = 0U; z < dataSize.z; ++z) {
                auto deviceMemPos = z * layout.depthPitch + offset.y * layout.rowPitch;
                auto dataMemPos = z * dataSize.x * dataSize.y;
                processFunc(&deviceBytes[deviceMemPos], dataMemPos, static_cast<std::size_t>(dataSize.x) * static_cast<std::size_t>(dataSize.y)); // NOLINT
            }
        }
        else {
            for (auto z = 0U; z < dataSize.z; ++z) {
                for (auto y = 0U; y < dataSize.y; ++y) {
                    auto deviceMemPos = z * layout.depthPitch + (static_cast<std::size_t>(offset.y) + y) * layout.rowPitch + offset.x;
                    auto dataMemPos = (z * dataSize.y + y) * dataSize.x;
                    processFunc(&deviceBytes[deviceMemPos], dataMemPos, dataSize.x); // NOLINT
                }
            }
        }

        device_->GetDevice().unmapMemory(*vkDeviceMemory_);
    }
}
