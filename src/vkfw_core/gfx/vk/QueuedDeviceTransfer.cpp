/**
 * @file   QueuedDeviceTransfer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.12
 *
 * @brief  Implementation of a queued device transfer operation.
 */

#include "gfx/vk/QueuedDeviceTransfer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/textures/HostTexture.h"
#include "gfx/vk/textures/DeviceTexture.h"
#include "gfx/vk/CommandBuffers.h"

namespace vkfw_core::gfx {

    QueuedDeviceTransfer::QueuedDeviceTransfer(const LogicalDevice* device, std::pair<std::uint32_t, std::uint32_t> transferQueue) :
        device_{ device },
        transferQueue_{ std::move(transferQueue) }
    {
    }


    QueuedDeviceTransfer::~QueuedDeviceTransfer()
    {
        try {
            if (!transferCmdBuffers_.empty()) { FinishTransfer(); }
        } catch (...) {
            spdlog::critical("Could not finish queued device transfer. Unknown exception.");
        }
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
        const vk::BufferUsageFlags& deviceBufferUsage, const vk::MemoryPropertyFlags& memoryFlags,
        const std::vector<std::uint32_t>& deviceBufferQueues, std::size_t bufferSize,
        std::size_t dataSize, const void* data)
    {
        AddStagingBuffer(dataSize, data);

        std::vector<std::uint32_t> queueFamilies;
        queueFamilies.reserve(deviceBufferQueues.size());
        for (auto queue : deviceBufferQueues) { queueFamilies.push_back(device_->GetQueueInfo(queue).familyIndex_); }
        auto deviceBuffer = std::make_unique<DeviceBuffer>(device_, vk::BufferUsageFlagBits::eTransferDst | deviceBufferUsage,
            memoryFlags, queueFamilies);
        deviceBuffer->InitializeBuffer(bufferSize);

        AddTransferToQueue(stagingBuffers_.back(), *deviceBuffer);

        return deviceBuffer;
    }

    std::unique_ptr<DeviceTexture> QueuedDeviceTransfer::CreateDeviceTextureWithData(
        const TextureDescriptor& textureDesc, const std::vector<std::uint32_t>& deviceBufferQueues,
        const glm::u32vec4& textureSize, std::uint32_t mipLevels, const glm::u32vec4& dataSize, const void* data)
    {
        AddStagingTexture(dataSize, mipLevels, textureDesc, data);

        std::vector<std::uint32_t> queueFamilies;
        queueFamilies.reserve(deviceBufferQueues.size());
        for (auto queue : deviceBufferQueues) { queueFamilies.push_back(device_->GetQueueInfo(queue).familyIndex_); }
        auto deviceTexture = std::make_unique<DeviceTexture>(device_, textureDesc, queueFamilies);
        deviceTexture->InitializeImage(textureSize, mipLevels);

        AddTransferToQueue(stagingTextures_.back(), *deviceTexture);

        return deviceTexture;
    }

    std::unique_ptr<DeviceBuffer> QueuedDeviceTransfer::CreateDeviceBufferWithData(
        const vk::BufferUsageFlags& deviceBufferUsage, const vk::MemoryPropertyFlags& memoryFlags,
        const std::vector<std::uint32_t>& deviceBufferQueues, std::size_t size, const void* data)
    {
        return CreateDeviceBufferWithData(deviceBufferUsage, memoryFlags, deviceBufferQueues, size, size, data);
    }

    std::unique_ptr<DeviceTexture> QueuedDeviceTransfer::CreateDeviceTextureWithData(
        const TextureDescriptor& textureDesc, const std::vector<std::uint32_t>& deviceBufferQueues,
        const glm::u32vec4& size, std::uint32_t mipLevels, const void* data)
    {
        return CreateDeviceTextureWithData(textureDesc, deviceBufferQueues, size, mipLevels, size, data);
    }

    void QueuedDeviceTransfer::TransferDataToBuffer(std::size_t dataSize, const void* data, const Buffer& dst, std::size_t dstOffset)
    {
        AddStagingBuffer(dataSize, data);
        AddTransferToQueue(stagingBuffers_.back(), 0, dst, dstOffset, dataSize);
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, std::size_t srcOffset, const Buffer& dst, std::size_t dstOffset, std::size_t copySize)
    {
        transferCmdBuffers_.push_back(src.CopyBufferAsync(srcOffset, dst, dstOffset, copySize, transferQueue_));
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, const Buffer& dst)
    {
        AddTransferToQueue(src, 0, dst, 0, src.GetSize());
    }

    void QueuedDeviceTransfer::AddTransferToQueue(Texture& src, Texture& dst)
    {
        auto imgCopyCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, transferQueue_.first);
        src.TransitionLayout(vk::ImageLayout::eTransferSrcOptimal, *imgCopyCmdBuffer);
        dst.TransitionLayout(vk::ImageLayout::eTransferDstOptimal, *imgCopyCmdBuffer);
        src.CopyImageAsync(0, glm::u32vec4(0), dst, 0, glm::u32vec4(0), src.GetSize(), *imgCopyCmdBuffer);
        dst.TransitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal, *imgCopyCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(device_, *imgCopyCmdBuffer, transferQueue_.first, transferQueue_.second);

        transferCmdBuffers_.push_back(std::move(imgCopyCmdBuffer));
    }

    void QueuedDeviceTransfer::FinishTransfer()
    {
        device_->GetQueue(transferQueue_.first, transferQueue_.second).waitIdle();
        transferCmdBuffers_.clear();
        stagingBuffers_.clear();
    }

    void QueuedDeviceTransfer::AddStagingBuffer(std::size_t dataSize, const void* data)
    {
        stagingBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc);
        stagingBuffers_.back().InitializeData(dataSize, data);
    }

    void QueuedDeviceTransfer::AddStagingTexture(const glm::u32vec4& size, std::uint32_t mipLevels,
        const TextureDescriptor& textureDesc, const void* data)
    {
        stagingTextures_.emplace_back(device_, TextureDescriptor::StagingTextureDesc(textureDesc));
        stagingTextures_.back().InitializeData(size, mipLevels, data);
    }
}
