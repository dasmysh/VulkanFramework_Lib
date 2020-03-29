/**
 * @file   MemoryGroup.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Declaration of a class managing multiple vulkan object for simultaneous memory usage.
 */

#pragma once

#include "main.h"
#include "DeviceMemoryGroup.h"
#include "core/type_traits.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/textures/HostTexture.h"
#include <variant>

namespace vkfw_core::gfx {

    class QueuedDeviceTransfer;

    class MemoryGroup final : public DeviceMemoryGroup
    {
    public:
        explicit MemoryGroup(const LogicalDevice* device, const vk::MemoryPropertyFlags& memoryFlags = vk::MemoryPropertyFlags());
        ~MemoryGroup() override;
        MemoryGroup(const MemoryGroup&) = delete;
        MemoryGroup& operator=(const MemoryGroup&) = delete;
        MemoryGroup(MemoryGroup&&) noexcept;
        MemoryGroup& operator=(MemoryGroup&&) noexcept;

        static constexpr unsigned int INVALID_INDEX = std::numeric_limits<unsigned int>::max();
        unsigned int AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size,
                         const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{}) override;
        unsigned int AddTextureToGroup(const TextureDescriptor& desc, const glm::u32vec4& size, std::uint32_t mipLevels,
                          const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{}) override;
        unsigned int AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, std::variant<void*, const void*> data,
                         const std::function<void(void*)>& deleter = nullptr,
                         const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        void AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize,
                                    std::variant<void*, const void*> data,
                                    const std::function<void(void*)>& deleter = nullptr);
        void AddDataToTextureInGroup(unsigned int textureIdx, const vk::ImageAspectFlags& aspectFlags, std::uint32_t mipLevel,
                                     std::uint32_t arrayLayer, const glm::u32vec3& size,
                                     std::variant<void*, const void*> data,
                                     const std::function<void(void*)>& deleter = nullptr);
        void FinalizeDeviceGroup() override;
        void TransferData(QueuedDeviceTransfer& transfer);
        void RemoveHostMemory();

        void FillUploadBufferCmdBuffer(unsigned int bufferIdx, vk::CommandBuffer cmdBuffer,
            std::size_t offset, std::size_t dataSize);

        HostBuffer* GetHostBuffer(unsigned int bufferIdx) { return &hostBuffers_[bufferIdx]; }
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
        /** Holds the Vulkan device memory for the host objects. */
        DeviceMemory hostMemory_;
        /** Holds the host buffers. */
        std::vector<HostBuffer> hostBuffers_;
        /** Holds the host images. */
        std::vector<HostTexture> hostImages_;

        struct BufferContentsDesc
        {
            /** The buffer index the contents belong to. */
            unsigned int bufferIdx_ = 0;
            /** The offset to copy the data to (in bytes). */
            std::size_t offset_ = 0;
            /** The size of the buffer data (in bytes). */
            std::size_t size_ = 0;
            /** Pointer to the data to copy. */
            std::variant<void*, const void*> data_;
            /** Deleter for the data_ element. */
            std::function<void(void*)> deleter_;

        };

        struct ImageContentsDesc
        {
            /** The image index the contents belong to. */
            unsigned int imageIdx_ = 0;
            /** The subresource aspect flags. */
            vk::ImageAspectFlags aspectFlags_;
            /** The MipMap level of the image contents. */
            std::uint32_t mipLevel_ = 0;
            /** The array layer of the image contents. */
            std::uint32_t arrayLayer_ = 0;
            /** The size of the image data (in bytes). */
            glm::u32vec3 size_ = glm::u32vec3{0};
            /** Pointer to the data to copy. */
            std::variant<void*, const void*> data_;
            /** Deleter for the data_ element. */
            std::function<void(void*)> deleter_;
        };

        /** Holds the offsets for the host memory objects. */
        std::vector<std::size_t> hostOffsets_;
        /** Holds the buffer contents that need to be transfered. */
        std::vector<BufferContentsDesc> bufferContents_;
        /** Holds the image contents that need to be transfered. */
        std::vector<ImageContentsDesc> imageContents_;
    };

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value, unsigned int> MemoryGroup::AddBufferToGroup(
        vk::BufferUsageFlags usage, const T& data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        return AddBufferToGroup(usage, byteSizeOf(data), data.data(), nullptr, queueFamilyIndices);
    }

    template <class T>
    std::enable_if_t<has_contiguous_memory<T>::value> MemoryGroup::AddDataToBufferInGroup(
        unsigned int bufferIdx, std::size_t offset, const T& data)
    {
        AddDataToBufferInGroup(bufferIdx, offset, byteSizeOf(data), data.data());
    }
}
