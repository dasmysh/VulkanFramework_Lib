/**
 * @file   BufferGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.27
 *
 * @brief  Implementation of the BufferGroup class.
 */

#include "gfx/vk/buffers/BufferGroup.h"
#include "gfx/vk/buffers/Buffer.h"
// ReSharper disable CppUnusedIncludeDirective
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/buffers/HostBuffer.h"
// ReSharper restore CppUnusedIncludeDirective
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vku::gfx {

    BufferGroup::BufferGroup(const LogicalDevice* device, const vk::MemoryPropertyFlags& memoryFlags) :
        device_{ device },
        memoryProperties_{ memoryFlags }
    {
    }


    BufferGroup::~BufferGroup()
    {
        hostBuffers_.clear();
        hostBufferMemory_.reset();
        deviceBuffers_.clear();
        deviceBufferMemory_.reset();
    }

    BufferGroup::BufferGroup(BufferGroup&& rhs) noexcept :
    device_{ rhs.device_ },
        deviceBufferMemory_{ std::move(rhs.deviceBufferMemory_) },
        hostBufferMemory_{ std::move(rhs.hostBufferMemory_) },
        deviceBuffers_{ std::move(rhs.deviceBuffers_) },
        hostBuffers_{ std::move(rhs.hostBuffers_) },
        memoryProperties_{ rhs.memoryProperties_ },
        bufferContents_{ std::move(rhs.bufferContents_) }
    {
    }

    BufferGroup& BufferGroup::operator=(BufferGroup&& rhs) noexcept
    {
        this->~BufferGroup();
        device_ = rhs.device_;
        deviceBufferMemory_ = std::move(rhs.deviceBufferMemory_);
        hostBufferMemory_ = std::move(rhs.hostBufferMemory_);
        deviceBuffers_ = std::move(rhs.deviceBuffers_);
        hostBuffers_ = std::move(rhs.hostBuffers_);
        memoryProperties_ = rhs.memoryProperties_;
        bufferContents_ = std::move(rhs.bufferContents_);
        return *this;
    }

    unsigned BufferGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferDst | usage, memoryProperties_, queueFamilyIndices);
        deviceBuffers_.back().InitializeBuffer(size, false);

        hostBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc, memoryProperties_, queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        bufferContents_.emplace_back(0, nullptr);
        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned BufferGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const void* data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        bufferContents_.back().first = size;
        bufferContents_.back().second = data;
        return idx;
    }

    void BufferGroup::FinalizeGroup(QueuedDeviceTransfer* transfer)
    {
        vk::MemoryAllocateInfo deviceAllocInfo;
        vk::MemoryAllocateInfo hostAllocInfo;
        std::vector<std::uint32_t> deviceSizes;
        std::vector<std::uint32_t> hostSizes;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            FillAllocationInfo(&hostBuffers_[i], hostAllocInfo, hostSizes);
            FillAllocationInfo(&deviceBuffers_[i], deviceAllocInfo, deviceSizes);
        }
        hostBufferMemory_ = device_->GetDevice().allocateMemoryUnique(hostAllocInfo);
        deviceBufferMemory_ = device_->GetDevice().allocateMemoryUnique(deviceAllocInfo);

        auto deviceOffset = 0U;
        auto hostOffset = 0U;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            device_->GetDevice().bindBufferMemory(hostBuffers_[i].GetBuffer(), *hostBufferMemory_, hostOffset);
            device_->GetDevice().bindBufferMemory(deviceBuffers_[i].GetBuffer(), *deviceBufferMemory_, deviceOffset);
            if (transfer != nullptr) {
                auto deviceMem = device_->GetDevice().mapMemory(*hostBufferMemory_, hostOffset, bufferContents_[i].first, vk::MemoryMapFlags());
                memcpy(deviceMem, bufferContents_[i].second, bufferContents_[i].first);
                device_->GetDevice().unmapMemory(*hostBufferMemory_);

                // hostBuffers_[i].UploadData(0, bufferContents_[i].first, bufferContents_[i].second);
                transfer->AddTransferToQueue(hostBuffers_[i], deviceBuffers_[i]);
            }
            hostOffset += hostSizes[i];
            deviceOffset += deviceSizes[i];

        }
    }

    void BufferGroup::FillAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        if (allocInfo.allocationSize == 0) {
            allocInfo.memoryTypeIndex = DeviceMemory::FindMemoryType(device_, memRequirements.memoryTypeBits,
                                                                     buffer->GetDeviceMemory().GetMemoryProperties());
        } else if (!DeviceMemory::CheckMemoryType(device_, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, buffer->GetDeviceMemory().GetMemoryProperties())) {
            spdlog::critical("BufferGroup memory type ({}) does not fit required memory type for buffer ({:#x}).", allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits);
            throw std::runtime_error("BufferGroup memory type does not fit required memory type for buffer.");
        }

        sizes.push_back(static_cast<unsigned int>(memRequirements.size));
        allocInfo.allocationSize += memRequirements.size;
    }
}
