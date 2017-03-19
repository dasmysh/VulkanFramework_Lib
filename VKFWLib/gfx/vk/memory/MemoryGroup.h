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

namespace vku::gfx {

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

        static constexpr unsigned int INVALID_INDEX = std::numeric_limits<unsigned int>::max();
        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        unsigned int AddTextureToGroup(const TextureDescriptor& desc,
            const glm::u32vec4& size, std::uint32_t mipLevels,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        unsigned int AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const void* data,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        void AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize, const void* data);
        void AddDataToTextureInGroup(unsigned int textureIdx, vk::ImageAspectFlags aspectFlags,
            std::uint32_t mipLevel, std::uint32_t arrayLayer, const glm::u32vec3& size, const void* data);
        void FinalizeGroup();
        void TransferData(QueuedDeviceTransfer& transfer);

        void FillUploadBufferCmdBuffer(unsigned int bufferIdx, vk::CommandBuffer cmdBuffer,
            std::size_t offset, std::size_t dataSize);

        DeviceBuffer* GetBuffer(unsigned int bufferIdx) { return &deviceBuffers_[bufferIdx]; }
        HostBuffer* GetHostBuffer(unsigned int bufferIdx) { return &hostBuffers_[bufferIdx]; }
        DeviceTexture* GetTexture(unsigned int textureIdx) { return &deviceImages_[textureIdx]; }
        HostTexture* GetHostTexture(unsigned int textureIdx) { return &hostImages_[textureIdx]; }
        DeviceMemory* GetHostMemory() { return &hostMemory_; }
        std::size_t GetHostBufferOffset(unsigned int bufferIdx) { return hostOffsets_[bufferIdx]; }
        std::size_t GetHostTextureOffset(unsigned int textureIdx) { return hostOffsets_[textureIdx + hostBuffers_.size()]; }


        template<class T> std::enable_if_t<has_contiguous_memory<T>::value, unsigned int> AddBufferToGroup(
            vk::BufferUsageFlags usage, const T& data,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<class T> std::enable_if_t<has_contiguous_memory<T>::value> AddDataToBufferInGroup(
            unsigned int bufferIdx, std::size_t offset, const T& data);

    private:
        std::size_t FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo) const;
        std::size_t FillImageAllocationInfo(Texture* image, vk::MemoryAllocateInfo& allocInfo) const;
        std::size_t FillAllocationInfo(const vk::MemoryRequirements& memRequirements, vk::MemoryPropertyFlags memProperties,
            vk::MemoryAllocateInfo& allocInfo) const;

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

        struct BufferContentsDesc
        {
            /** The buffer index the contents belong to. */
            unsigned int bufferIdx_;
            /** The offset to copy the data to (in bytes). */
            std::size_t offset_;
            /** The size of the buffer data (in bytes). */
            std::size_t size_;
            /** Pointer to the data to copy. */
            const void* data_;

        };

        struct ImageContentsDesc
        {
            /** The image index the contents belong to. */
            unsigned int imageIdx_;
            /** The subresource aspect flags. */
            vk::ImageAspectFlags aspectFlags_;
            /** The MipMap level of the image contents. */
            std::uint32_t mipLevel_;
            /** The array layer of the image contents. */
            std::uint32_t arrayLayer_;
            /** The size of the image data (in bytes). */
            glm::u32vec3 size_;
            /** Pointer to the data to copy. */
            const void* data_;
        };

        /** Holds the offsets for the host memory objects. */
        std::vector<std::size_t> hostOffsets_;
        /** Holds the offsets for the device memory objects. */
        std::vector<std::size_t> deviceOffsets_;
        /** Holds the buffer contents that need to be transfered. */
        std::vector<BufferContentsDesc> bufferContents_;
        /** Holds the image contents that need to be transfered. */
        std::vector<ImageContentsDesc> imageContents_;
    };

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value, unsigned int> MemoryGroup::AddBufferToGroup(
        vk::BufferUsageFlags usage, const T& data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        return AddBufferToGroup(usage, byteSizeOf(data), data.data(), queueFamilyIndices);
    }

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value> MemoryGroup::AddDataToBufferInGroup(
        unsigned int bufferIdx, std::size_t offset, const T& data)
    {
        AddDataToBufferInGroup(bufferIdx, offset, byteSizeOf(data), data.data());
    }
}
