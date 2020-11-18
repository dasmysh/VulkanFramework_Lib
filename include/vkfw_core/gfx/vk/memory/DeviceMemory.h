/**
 * @file   DeviceMemory.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Declaration of vulkan device memory objects.
 */

#pragma once

#include "main.h"

#include <glm/gtc/type_precision.hpp>

#include <core/function_view.h>

namespace vkfw_core::gfx {

    class Buffer;
    class Texture;

    class DeviceMemory final
    {
    public:
        DeviceMemory(const LogicalDevice* device, const vk::MemoryPropertyFlags& properties);
        DeviceMemory(const LogicalDevice* device, vk::MemoryRequirements memRequirements,
                     const vk::MemoryPropertyFlags& properties);
        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory& operator=(const DeviceMemory&) = delete;
        DeviceMemory(DeviceMemory&&) noexcept;
        DeviceMemory& operator=(DeviceMemory&&) noexcept;
        ~DeviceMemory();

        void InitializeMemory(const vk::MemoryRequirements& memRequirements, bool shaderDeviceAddress = false);
        void InitializeMemory(const vk::MemoryAllocateInfo& memAllocateInfo);

        void BindToBuffer(Buffer& buffer, std::size_t offset) const;
        void BindToTexture(Texture& texture, std::size_t offset) const;

        void CopyToHostMemory(std::size_t offset, std::size_t size, const void* data) const;
        void CopyToHostMemory(std::size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, const void* data) const;

        void CopyFromHostMemory(std::size_t offset, std::size_t size, void* data) const;
        void CopyFromHostMemory(std::size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize, void* data) const;

        [[nodiscard]] vk::MemoryPropertyFlags GetMemoryProperties() const { return m_memoryProperties; }

        static std::uint32_t FindMemoryType(const LogicalDevice* device, std::uint32_t typeFilter,
                                            const vk::MemoryPropertyFlags& properties);
        static bool CheckMemoryType(const LogicalDevice* device, std::uint32_t typeToCheck, std::uint32_t typeFilter,
                                    const vk::MemoryPropertyFlags& properties);

    private:
        static bool CheckMemoryType(const vk::PhysicalDeviceMemoryProperties& memProperties, std::uint32_t typeToCheck,
                                    std::uint32_t typeFilter, const vk::MemoryPropertyFlags& properties);
        void MapAndProcess(std::size_t offset, std::size_t size,
                           const function_view<void(void* deviceMem, std::size_t size)>& processFunc) const;
        void MapAndProcess(std::size_t offsetToTexture, const glm::u32vec3& offset,
            const vk::SubresourceLayout& layout, const glm::u32vec3& dataSize,
            const function_view<void(void* deviceMem, std::size_t offset, std::size_t size)>& processFunc) const;

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the Vulkan device memory. */
        vk::UniqueDeviceMemory m_vkDeviceMemory;
        /** Holds the current size of the memory in bytes. */
        std::size_t m_size;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags m_memoryProperties;
    };
}
