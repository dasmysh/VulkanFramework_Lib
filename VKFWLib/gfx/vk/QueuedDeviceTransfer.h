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
            size_t size, const void* data);
        void AddTransferToQueue(const Buffer& src, const Buffer& dst);

        void FinishTransfer();
        
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> CreateDeviceBufferWithData(
            vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags,
                const std::vector<uint32_t>& deviceBufferQueues, const T& data);

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the transfer queue used. */
        std::pair<uint32_t, uint32_t> transferQueue_;
        /** Holds all staging buffers. */
        std::vector<HostBuffer> stagingBuffers_;
        /** Holds all command buffers for transfer. */
        std::vector<vk::CommandBuffer> transferCmdBuffers_;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value, std::unique_ptr<DeviceBuffer>> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& deviceBufferQueues, const T& data)
    {
        return CreateDeviceBufferWithData(deviceBufferUsage, memoryFlags, deviceBufferQueues,
            static_cast<size_t>(sizeof(T::value_type) * data.size()), data.data());
    }
}}
