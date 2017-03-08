/**
 * @file   QueuedDeviceTransfer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.12
 *
 * @brief  Declaration of a queued device transfer operation.
 */

#pragma once

#include "main.h"
#include "core/type_traits.h"

namespace vku { namespace gfx {

    class DeviceBuffer;
    class HostBuffer;
    class Buffer;
    class DeviceTexture;
    class HostTexture;
    class Texture;
    struct TextureDescriptor;

    class QueuedDeviceTransfer final
    {
    public:
        QueuedDeviceTransfer(const LogicalDevice* device, std::pair<uint32_t, uint32_t> transferQueue);
        ~QueuedDeviceTransfer();
        QueuedDeviceTransfer(const QueuedDeviceTransfer&) = delete;
        QueuedDeviceTransfer& operator=(const QueuedDeviceTransfer&) = delete;
        QueuedDeviceTransfer(QueuedDeviceTransfer&&) noexcept;
        QueuedDeviceTransfer& operator=(QueuedDeviceTransfer&&) noexcept;

        std::unique_ptr<DeviceBuffer> CreateDeviceBufferWithData(vk::BufferUsageFlags deviceBufferUsage,
            vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& deviceBufferQueues,
            size_t bufferSize, size_t dataSize, const void* data);
        std::unique_ptr<DeviceTexture> CreateDeviceTextureWithData(const TextureDescriptor& textureDesc,
            const std::vector<uint32_t>& deviceBufferQueues, const glm::u32vec4& textureSize,
            uint32_t mipLevels, const glm::u32vec4& dataSize, const void* data);
        std::unique_ptr<DeviceBuffer> CreateDeviceBufferWithData(vk::BufferUsageFlags deviceBufferUsage,
            vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& deviceBufferQueues,
            size_t size, const void* data);
        std::unique_ptr<DeviceTexture> CreateDeviceTextureWithData(const TextureDescriptor& textureDesc,
            const std::vector<uint32_t>& deviceBufferQueues, const glm::u32vec4& size,
            uint32_t mipLevels, const void* data);

        void TransferDataToBuffer(size_t dataSize, const void* data, const Buffer& dst, size_t dstOffset);

        void AddTransferToQueue(const Buffer& src, size_t srcOffset, const Buffer& dst, size_t dstOffset, size_t copySize);
        void AddTransferToQueue(const Buffer& src, const Buffer& dst);
        void AddTransferToQueue(const Texture& src, const Texture& dst);

        void FinishTransfer();

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> CreateDeviceBufferWithData(
            vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags,
                const std::vector<uint32_t>& deviceBufferQueues, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> CreateDeviceBufferWithData(
            vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags,
            const std::vector<uint32_t>& deviceBufferQueues, size_t bufferSize, const T& data);

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> TransferDataToBuffer(const T& data, const Buffer& dst, size_t dstOffset);

    private:
        void AddStagingBuffer(size_t dataSize, const void* data);
        void AddStagingTexture(const glm::u32vec4& size, uint32_t mipLevels,
            const TextureDescriptor& textureDesc, const void* data);

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the transfer queue used. */
        std::pair<uint32_t, uint32_t> transferQueue_;
        /** Holds all staging buffers. */
        std::vector<HostBuffer> stagingBuffers_;
        /** Holds all staging textures. */
        std::vector<HostTexture> stagingTextures_;
        /** Holds all command buffers for transfer. */
        std::vector<vk::CommandBuffer> transferCmdBuffers_;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& deviceBufferQueues, size_t bufferSize, const T& data)
    {
        return CreateDeviceBufferWithData(deviceBufferUsage, memoryFlags, deviceBufferQueues,
            bufferSize, byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& deviceBufferQueues, const T& data)
    {
        return CreateDeviceBufferWithData(deviceBufferUsage, memoryFlags, deviceBufferQueues,
            byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> QueuedDeviceTransfer::TransferDataToBuffer(const T& data, const Buffer& dst, size_t dstOffset)
    {
        TransferDataToBuffer(byteSizeOf(data), data.data(), dst, dstOffset);
    }
}}
