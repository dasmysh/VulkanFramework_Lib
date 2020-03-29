/**
 * @file   Buffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general Vulkan buffer.
 */

#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/buffers/BufferGroup.h"
#include "gfx/vk/CommandBuffers.h"

namespace vkfw_core::gfx {

    Buffer::Buffer(const LogicalDevice* device, const vk::BufferUsageFlags& usage,
        const vk::MemoryPropertyFlags& memoryFlags, std::vector<std::uint32_t> queueFamilyIndices) :
        device_{ device },
        bufferDeviceMemory_{ device, memoryFlags },
        size_{ 0 },
        usage_{ usage },
        queueFamilyIndices_{ std::move(queueFamilyIndices) }
    {
    }

    Buffer::~Buffer() = default;

    Buffer::Buffer(Buffer&& rhs) noexcept :
        device_{ rhs.device_ },
        buffer_{ std::move(rhs.buffer_) },
        bufferDeviceMemory_{ std::move(rhs.bufferDeviceMemory_) },
        size_{ rhs.size_ },
        usage_{ rhs.usage_ },
        queueFamilyIndices_{ std::move(rhs.queueFamilyIndices_) }
    {
        rhs.size_ = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        device_ = rhs.device_;
        buffer_ = std::move(rhs.buffer_);
        bufferDeviceMemory_ = std::move(rhs.bufferDeviceMemory_);
        size_ = rhs.size_;
        usage_ = rhs.usage_;
        queueFamilyIndices_ = std::move(rhs.queueFamilyIndices_);
        rhs.size_ = 0;
        return *this;
    }

    void Buffer::InitializeBuffer(std::size_t size, bool initMemory)
    {
        this->~Buffer();

        size_ = size;
        vk::BufferCreateInfo bufferCreateInfo{ vk::BufferCreateFlags(), static_cast<vk::DeviceSize>(size_), usage_, vk::SharingMode::eExclusive };
        if (!queueFamilyIndices_.empty()) {
            bufferCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(queueFamilyIndices_.size()));
            bufferCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }
        if (queueFamilyIndices_.size() > 1) { bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive); }
        buffer_ = device_->GetDevice().createBufferUnique(bufferCreateInfo);

        if (initMemory) {
            auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(*buffer_);
            bufferDeviceMemory_.InitializeMemory(memRequirements);
            bufferDeviceMemory_.BindToBuffer(*this, 0);
        }
    }

    void Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer & dstBuffer, std::size_t dstOffset,
        std::size_t size, vk::CommandBuffer cmdBuffer) const
    {
        assert(usage_ & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.usage_ & vk::BufferUsageFlagBits::eTransferDst);
        assert(srcOffset + size <= size_);
        assert(dstOffset + size <= dstBuffer.size_);

        vk::BufferCopy copyRegion{ srcOffset, dstOffset, size };
        cmdBuffer.copyBuffer(*buffer_, *dstBuffer.buffer_, copyRegion);
    }

    vk::UniqueCommandBuffer Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset,
        std::size_t size, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        auto transferCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, copyQueueIdx.first);
        CopyBufferAsync(srcOffset, dstBuffer, dstOffset, size, *transferCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(device_, *transferCmdBuffer, copyQueueIdx.first, copyQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    vk::UniqueCommandBuffer Buffer::CopyBufferAsync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
        vk::Fence fence) const
    {
        return CopyBufferAsync(0, dstBuffer, 0, size_, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Buffer::CopyBufferSync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyBufferAsync(dstBuffer, copyQueueIdx);
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();
    }
}
