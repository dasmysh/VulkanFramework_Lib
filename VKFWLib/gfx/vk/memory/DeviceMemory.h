/**
 * @file   DeviceMemory.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Declaration of vulkan device memory objects.
 */

#pragma once

#include "main.h"

namespace vku { namespace gfx {

    class Buffer;
    class Texture;

    class DeviceMemory
    {
    public:
        DeviceMemory(const LogicalDevice* device, vk::MemoryPropertyFlags properties);
        DeviceMemory(const LogicalDevice* device, vk::MemoryRequirements memRequirements, vk::MemoryPropertyFlags properties);
        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory& operator=(const DeviceMemory&) = delete;
        DeviceMemory(DeviceMemory&&) noexcept;
        DeviceMemory& operator=(DeviceMemory&&) noexcept;
        ~DeviceMemory();

        void InitializeMemory(const vk::MemoryRequirements& memRequirements);
        void InitializeMemory(const vk::MemoryAllocateInfo& memAllocateInfo);

        void BindToBuffer(Buffer& buffer, size_t offset) const;
        void BindToTexture(Texture& texture, size_t offset) const;

        void CopyToHostMemory(size_t offset, size_t size, const void* data) const;
        void CopyToHostMemory(size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, const void* data) const;

        void CopyFromHostMemory(size_t offset, size_t size, void* data) const;
        void CopyFromHostMemory(size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, void* data) const;

        vk::MemoryPropertyFlags GetMemoryProperties() const { return memoryProperties_; }

        static uint32_t FindMemoryType(const LogicalDevice* device, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        static bool CheckMemoryType(const LogicalDevice* device, uint32_t typeToCheck, uint32_t typeFilter,
            vk::MemoryPropertyFlags properties);

    private:
        static bool CheckMemoryType(const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeToCheck,
            uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        void MapAndProcess(size_t offset, size_t size, const std::function<void(void* deviceMem, size_t size)>& processFunc) const;
        void MapAndProcess(size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize,
            const std::function<void(void* deviceMem, size_t offset, size_t size)>& processFunc) const;

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan device memory. */
        vk::DeviceMemory vkDeviceMemory_;
        /** Holds the current size of the memory in bytes. */
        size_t size_;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_;
    };
}}
