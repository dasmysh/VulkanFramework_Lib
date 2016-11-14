/**
 * @file   QueuedDeviceTransfer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.12
 *
 * @brief  Implementation of a queued device transfer operation.
 */

#include "QueuedDeviceTransfer.h"
#include "HostBuffer.h"
#include "DeviceBuffer.h"

namespace vku { namespace gfx {

    QueuedDeviceTransfer::QueuedDeviceTransfer(const LogicalDevice* device, std::pair<uint32_t, uint32_t> transferQueue) :
        device_{ device },
        transferQueue_{ transferQueue }
    {
    }


    QueuedDeviceTransfer::~QueuedDeviceTransfer() = default;

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
        const std::vector<uint32_t>& deviceBufferQueues, size_t size, const void* data)
    {
        stagingBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc);
        stagingBuffers_.back().InitializeData(size, data);

        std::vector<uint32_t> queueFamilies;
        for (auto queue : deviceBufferQueues) queueFamilies.push_back(device_->GetQueueInfo(queue).familyIndex_);
        auto deviceBuffer = std::make_unique<DeviceBuffer>(device_, vk::BufferUsageFlagBits::eTransferDst | deviceBufferUsage,
            memoryFlags, queueFamilies);
        deviceBuffer->InitializeBuffer(size);

        AddTransferToQueue(stagingBuffers_.back(), *deviceBuffer);

        return deviceBuffer;
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, const Buffer& dst)
    {
        transferCmdBuffers_.push_back(src.CopyBufferAsync(dst, transferQueue_));
    }

    void QueuedDeviceTransfer::FinishTransfer()
    {
        device_->GetQueue(transferQueue_.first, transferQueue_.second).waitIdle();
        for (auto cmdBuffer : transferCmdBuffers_) device_->GetDevice().freeCommandBuffers(device_->GetCommandPool(transferQueue_.first), cmdBuffer);

        transferCmdBuffers_.clear();
        stagingBuffers_.clear();
    }
}}
