/**
 * @file   MemoryGroup.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Declaration of a class managing multiple vulkan object for simultaneous memory usage.
 */

#pragma once

#include "main.h"
#include "gfx/vk/LogicalDevice.h"
#include "DeviceMemory.h"
#include "core/type_traits.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/textures/DeviceTexture.h"
#include "gfx/vk/textures/HostTexture.h"

namespace vku { namespace gfx {

    class QueuedDeviceTransfer;

    class MemoryGroup
    {
    public:
        explicit MemoryGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags());
        virtual ~MemoryGroup();
        MemoryGroup(const MemoryGroup&) = delete;
        MemoryGroup& operator=(const MemoryGroup&) = delete;
        MemoryGroup(MemoryGroup&&) noexcept;
        MemoryGroup& operator=(MemoryGroup&&) noexcept;

        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, size_t size,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        unsigned int AddTextureToGroup(const TextureDescriptor& desc,
            const glm::u32vec4& size, uint32_t mipLevels,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, size_t size, const void* data,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        void AddDataToTextureInGroup(unsigned int textureIdx, vk::ImageAspectFlags aspectFlags,
            uint32_t mipLevel, uint32_t arrayLayer, const glm::u32vec3& size, const void* data);
        void FinalizeGroup(QueuedDeviceTransfer* transfer = nullptr);

        DeviceBuffer* GetBuffer(unsigned int bufferIdx) { return &deviceBuffers_[bufferIdx]; }
        DeviceTexture* GetTexture(unsigned int textureIdx) { return &deviceImages_[textureIdx]; }

        template<class T> std::enable_if_t<has_contiguous_memory<T>::value> AddBufferToGroup(vk::BufferUsageFlags usage, const T& data,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});

    private:
        void FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const;
        void FillImageAllocationInfo(Texture* image, vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const;
        void FillAllocationInfo(const vk::MemoryRequirements& memRequirements, vk::MemoryPropertyFlags memProperties,
            vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const;

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan device memory for the device objects. */
        DeviceMemory deviceMemory_;
        /** Holds the Vulkan device memory for the host objects. */
        DeviceMemory hostMemory_;
        /** Holds the device buffers. */
        std::vector<DeviceBuffer> deviceBuffers_;
        /** Holds the device images. */
        std::vector<DeviceTexture> deviceImages_;
        /** Holds the host buffers. */
        std::vector<HostBuffer> hostBuffers_;
        /** Holds the host images. */
        std::vector<HostTexture> hostImages_;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_;

        struct ImageContensDesc
        {
            /** The image index the contents belong to. */
            size_t imageIdx_;
            /** The subresource aspect flags. */
            vk::ImageAspectFlags aspectFlags_;
            /** The MipMap level of the image contents. */
            uint32_t mipLevel_;
            /** The array layer of the image contents. */
            uint32_t arrayLayer_;
            /** The size of the image data (in bytes). */
            glm::u32vec3 size_;
            /** Pointer to the data to copy. */
            const void* data_;
        };

        /** Holds the buffer contents that need to be transfered. */
        std::vector<std::pair<size_t, const void*>> bufferContents_;
        /** Holds the image contents that need to be transfered. */
        std::vector<ImageContensDesc> imageContents_;
    };

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value> MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage,
        const T& data, const std::vector<uint32_t>& queueFamilyIndices)
    {
        AddBufferToGroup(usage, byteSizeOf(data), data.data(), queueFamilyIndices);
    }
}}
