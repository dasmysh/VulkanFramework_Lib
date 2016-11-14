/**
 * @file   Buffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general Vulkan buffer.
 */

#include "Buffer.h"
#include "LogicalDevice.h"

namespace vku { namespace gfx {

    Buffer::Buffer(const LogicalDevice* device, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<uint32_t>& queueFamilyIndices) :
        device_{ device },
        size_{ 0 },
        usage_{ usage },
        memoryProperties_{ memoryFlags },
        queueFamilyIndices_{ queueFamilyIndices }
    {
    }

    Buffer::~Buffer()
    {
        if (buffer_) device_->GetDevice().destroyBuffer(buffer_);
        buffer_ = vk::Buffer();
        if (bufferDeviceMemory_) device_->GetDevice().freeMemory(bufferDeviceMemory_);
        bufferDeviceMemory_ = vk::DeviceMemory();
    }

    Buffer::Buffer(Buffer&& rhs) noexcept :
        device_{ rhs.device_ },
        buffer_{ rhs.buffer_ },
        bufferDeviceMemory_{ rhs.bufferDeviceMemory_ },
        size_{ rhs.size_ },
        usage_{ rhs.usage_ }
    {
        rhs.buffer_ = vk::Buffer();
        rhs.bufferDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        device_ = rhs.device_;
        buffer_ = rhs.buffer_;
        bufferDeviceMemory_ = rhs.bufferDeviceMemory_;
        size_ = rhs.size_;
        usage_ = rhs.usage_;
        rhs.buffer_ = vk::Buffer();
        rhs.bufferDeviceMemory_ = vk::DeviceMemory();
        rhs.size_ = 0;
        return *this;
    }

    uint32_t Buffer::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        auto memProperties = device_->GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Buffer::InitializeBuffer(size_t size)
    {
        this->~Buffer();

        size_ = size;
        vk::BufferCreateInfo bufferCreateInfo{ vk::BufferCreateFlags(), static_cast<vk::DeviceSize>(size_), usage_, vk::SharingMode::eExclusive };
        if (queueFamilyIndices_.size() > 0) {
            bufferCreateInfo.setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices_.size()));
            bufferCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }
        if (queueFamilyIndices_.size() > 1) bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
        buffer_ = device_->GetDevice().createBuffer(bufferCreateInfo);

        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer_);
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, FindMemoryType(memRequirements.memoryTypeBits, memoryProperties_) };
        bufferDeviceMemory_ = device_->GetDevice().allocateMemory(allocInfo);
        device_->GetDevice().bindBufferMemory(buffer_, bufferDeviceMemory_, 0);
    }

    vk::CommandBuffer Buffer::CopyBufferAsync(const Buffer& dstBuffer, std::pair<uint32_t, uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
        vk::Fence fence) const
    {
        assert(usage_ & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.usage_ & vk::BufferUsageFlagBits::eTransferDst);
        assert(size_ <= dstBuffer.size_);

        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device_->GetCommandPool(copyQueueIdx.first) , vk::CommandBufferLevel::ePrimary, 1 };
        auto transferCmdBuffers = device_->GetDevice().allocateCommandBuffers(cmdBufferallocInfo);

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        transferCmdBuffers[0].begin(beginInfo);
        vk::BufferCopy copyRegion{ 0, 0, size_ };
        transferCmdBuffers[0].copyBuffer(buffer_, dstBuffer.buffer_, copyRegion);
        transferCmdBuffers[0].end();

        vk::SubmitInfo submitInfo{ static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, static_cast<uint32_t>(transferCmdBuffers.size()), transferCmdBuffers.data(),
            static_cast<uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).submit(submitInfo, fence);

        return transferCmdBuffers[0];
    }

    void Buffer::CopyBufferSync(const Buffer& dstBuffer, std::pair<uint32_t, uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyBufferAsync(dstBuffer, copyQueueIdx);
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();

        device_->GetDevice().freeCommandBuffers(device_->GetCommandPool(copyQueueIdx.first), cmdBuffer);
    }
}}
