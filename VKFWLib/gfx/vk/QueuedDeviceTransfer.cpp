/**
 * @file   QueuedDeviceTransfer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.12
 *
 * @brief  Implementation of a queued device transfer operation.
 */

#include "QueuedDeviceTransfer.h"
#include "buffers/HostBuffer.h"
#include "buffers/DeviceBuffer.h"

namespace vku { namespace gfx {

    QueuedDeviceTransfer::QueuedDeviceTransfer(const LogicalDevice* device, std::pair<uint32_t, uint32_t> transferQueue) :
        device_{ device },
        transferQueue_{ transferQueue }
    {
    }


    QueuedDeviceTransfer::~QueuedDeviceTransfer()
    {
        if (!transferCmdBuffers_.empty()) FinishTransfer();
    }

    QueuedDeviceTransfer::QueuedDeviceTransfer(QueuedDeviceTransfer&& rhs) noexcept :
        device_{ rhs.device_ },
        transferQueue_{ std::move(rhs.transferQueue_) },
        stagingBuffers_{ std::move(rhs.stagingBuffers_) },
        transferCmdBuffers_{ std::move(rhs.transferCmdBuffers_) }
    {
    }

    QueuedDeviceTransfer& QueuedDeviceTransfer::operator=(QueuedDeviceTransfer&& rhs) noexcept
    {
        this->~QueuedDeviceTransfer();
        device_ = rhs.device_;
        transferQueue_ = std::move(rhs.transferQueue_);
        stagingBuffers_ = std::move(rhs.stagingBuffers_);
        transferCmdBuffers_ = std::move(rhs.transferCmdBuffers_);
        return *this;
    }

    std::unique_ptr<DeviceBuffer> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags,
        const std::vector<uint32_t>& deviceBufferQueues, size_t bufferSize,
        size_t dataSize, const void* data)
    {
        AddStagingBuffer(dataSize, data);

        std::vector<uint32_t> queueFamilies;
        for (auto queue : deviceBufferQueues) queueFamilies.push_back(device_->GetQueueInfo(queue).familyIndex_);
        auto deviceBuffer = std::make_unique<DeviceBuffer>(device_, vk::BufferUsageFlagBits::eTransferDst | deviceBufferUsage,
            memoryFlags, queueFamilies);
        deviceBuffer->InitializeBuffer(bufferSize);

        AddTransferToQueue(stagingBuffers_.back(), *deviceBuffer);

        return deviceBuffer;
    }

    std::unique_ptr<DeviceBuffer> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        vk::BufferUsageFlags deviceBufferUsage, vk::MemoryPropertyFlags memoryFlags,
        const std::vector<uint32_t>& deviceBufferQueues, size_t size, const void* data)
    {
        return CreateDeviceBufferWithData(deviceBufferUsage, memoryFlags, deviceBufferQueues, size, size, data);
    }

    void QueuedDeviceTransfer::TransferDataToBuffer(size_t dataSize, const void* data, const Buffer& dst, size_t dstOffset)
    {
        AddStagingBuffer(dataSize, data);
        AddTransferToQueue(stagingBuffers_.back(), 0, dst, dstOffset, dataSize);
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, size_t srcOffset, const Buffer& dst, size_t dstOffset, size_t copySize)
    {
        transferCmdBuffers_.push_back(src.CopyBufferAsync(srcOffset, dst, dstOffset, copySize, transferQueue_));
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, const Buffer& dst)
    {
        AddTransferToQueue(src, 0, dst, 0, src.GetSize());
    }

    void QueuedDeviceTransfer::FinishTransfer()
    {
        device_->GetQueue(transferQueue_.first, transferQueue_.second).waitIdle();
        for (auto cmdBuffer : transferCmdBuffers_) device_->GetDevice().freeCommandBuffers(device_->GetCommandPool(transferQueue_.first), cmdBuffer);

        transferCmdBuffers_.clear();
        stagingBuffers_.clear();
    }

    void QueuedDeviceTransfer::AddStagingBuffer(size_t dataSize, const void* data)
    {
        stagingBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc);
        stagingBuffers_.back().InitializeData(dataSize, data);
    }
}}
