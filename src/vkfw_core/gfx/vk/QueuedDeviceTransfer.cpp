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
        m_device{ device },
        m_transferQueue{ std::move(transferQueue) }
    {
    }


    QueuedDeviceTransfer::~QueuedDeviceTransfer()
    {
        try {
            if (!m_transferCmdBuffers.empty()) { FinishTransfer(); }
        } catch (...) {
            spdlog::critical("Could not finish queued device transfer. Unknown exception.");
        }
    }

    QueuedDeviceTransfer::QueuedDeviceTransfer(QueuedDeviceTransfer&& rhs) noexcept :
    m_device{ rhs.m_device },
        m_transferQueue{ std::move(rhs.m_transferQueue) },
        m_stagingBuffers{ std::move(rhs.m_stagingBuffers) },
        m_transferCmdBuffers{ std::move(rhs.m_transferCmdBuffers) }
    {
    }

    QueuedDeviceTransfer& QueuedDeviceTransfer::operator=(QueuedDeviceTransfer&& rhs) noexcept
    {
        this->~QueuedDeviceTransfer();
        m_device = rhs.m_device;
        m_transferQueue = std::move(rhs.m_transferQueue);
        m_stagingBuffers = std::move(rhs.m_stagingBuffers);
        m_transferCmdBuffers = std::move(rhs.m_transferCmdBuffers);
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
        for (auto queue : deviceBufferQueues) { queueFamilies.push_back(m_device->GetQueueInfo(queue).m_familyIndex); }
        auto deviceBuffer = std::make_unique<DeviceBuffer>(
            m_device, vk::BufferUsageFlagBits::eTransferDst | deviceBufferUsage,
            memoryFlags, queueFamilies);
        deviceBuffer->InitializeBuffer(bufferSize);

        AddTransferToQueue(m_stagingBuffers.back(), *deviceBuffer);

        return deviceBuffer;
    }

    std::unique_ptr<DeviceTexture> QueuedDeviceTransfer::CreateDeviceTextureWithData(
        const TextureDescriptor& textureDesc, const std::vector<std::uint32_t>& deviceBufferQueues,
        const glm::u32vec4& textureSize, std::uint32_t mipLevels, const glm::u32vec4& dataSize, const void* data)
    {
        AddStagingTexture(dataSize, mipLevels, textureDesc, data);

        std::vector<std::uint32_t> queueFamilies;
        queueFamilies.reserve(deviceBufferQueues.size());
        for (auto queue : deviceBufferQueues) { queueFamilies.push_back(m_device->GetQueueInfo(queue).m_familyIndex); }
        auto deviceTexture = std::make_unique<DeviceTexture>(m_device, textureDesc, queueFamilies);
        deviceTexture->InitializeImage(textureSize, mipLevels);

        AddTransferToQueue(m_stagingTextures.back(), *deviceTexture);

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
        AddTransferToQueue(m_stagingBuffers.back(), 0, dst, dstOffset, dataSize);
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, std::size_t srcOffset, const Buffer& dst, std::size_t dstOffset, std::size_t copySize)
    {
        m_transferCmdBuffers.push_back(src.CopyBufferAsync(srcOffset, dst, dstOffset, copySize, m_transferQueue));
    }

    void QueuedDeviceTransfer::AddTransferToQueue(const Buffer& src, const Buffer& dst)
    {
        AddTransferToQueue(src, 0, dst, 0, src.GetSize());
    }

    void QueuedDeviceTransfer::AddTransferToQueue(Texture& src, Texture& dst)
    {
        auto imgCopyCmdBuffer = CommandBuffers::beginSingleTimeSubmit(m_device, m_transferQueue.first);
        src.TransitionLayout(vk::ImageLayout::eTransferSrcOptimal, *imgCopyCmdBuffer);
        dst.TransitionLayout(vk::ImageLayout::eTransferDstOptimal, *imgCopyCmdBuffer);
        src.CopyImageAsync(0, glm::u32vec4(0), dst, 0, glm::u32vec4(0), src.GetSize(), *imgCopyCmdBuffer);
        dst.TransitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal, *imgCopyCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(m_device, *imgCopyCmdBuffer, m_transferQueue.first, m_transferQueue.second);

        m_transferCmdBuffers.push_back(std::move(imgCopyCmdBuffer));
    }

    void QueuedDeviceTransfer::FinishTransfer()
    {
        m_device->GetQueue(m_transferQueue.first, m_transferQueue.second).waitIdle();
        m_transferCmdBuffers.clear();
        m_stagingBuffers.clear();
    }

    void QueuedDeviceTransfer::AddStagingBuffer(std::size_t dataSize, const void* data)
    {
        m_stagingBuffers.emplace_back(m_device, vk::BufferUsageFlagBits::eTransferSrc);
        m_stagingBuffers.back().InitializeData(dataSize, data);
    }

    void QueuedDeviceTransfer::AddStagingTexture(const glm::u32vec4& size, std::uint32_t mipLevels,
        const TextureDescriptor& textureDesc, const void* data)
    {
        m_stagingTextures.emplace_back(m_device, TextureDescriptor::StagingTextureDesc(textureDesc));
        m_stagingTextures.back().InitializeData(size, mipLevels, data);
    }
}
