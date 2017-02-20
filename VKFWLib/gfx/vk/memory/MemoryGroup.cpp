/**
 * @file   MemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of the MemoryGroup class.
 */

#include "MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"
#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/textures/Texture.h"

namespace vku { namespace gfx {

    MemoryGroup::MemoryGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags) :
        device_{ device },
        deviceMemory_{device, memoryFlags},
        hostMemory_{ device, memoryFlags },
        memoryProperties_{ memoryFlags }
    {
    }


    MemoryGroup::~MemoryGroup()
    {
        hostBuffers_.clear();
        deviceBuffers_.clear();
    }

    MemoryGroup::MemoryGroup(MemoryGroup&& rhs) noexcept :
        device_{ rhs.device_ },
        deviceMemory_{ std::move(rhs.deviceMemory_) },
        hostMemory_{ std::move(rhs.hostMemory_) },
        deviceBuffers_{ std::move(rhs.deviceBuffers_) },
        deviceImages_{ std::move(rhs.deviceImages_) },
        hostBuffers_{ std::move(rhs.hostBuffers_) },
        hostImages_{ std::move(rhs.hostImages_) },
        memoryProperties_{ rhs.memoryProperties_ },
        bufferContents_{ std::move(rhs.bufferContents_) },
        imageContents_{ std::move(rhs.imageContents_) }
    {
    }

        MemoryGroup& MemoryGroup::operator=(MemoryGroup&& rhs) noexcept
    {
        this->~MemoryGroup();
        device_ = rhs.device_;
        deviceMemory_ = std::move(rhs.deviceMemory_);
        hostMemory_ = std::move(rhs.hostMemory_);
        deviceBuffers_ = std::move(rhs.deviceBuffers_);
        deviceImages_ = std::move(rhs.deviceImages_);
        hostBuffers_ = std::move(rhs.hostBuffers_);
        hostImages_ = std::move(rhs.hostImages_);
        memoryProperties_ = rhs.memoryProperties_;
        bufferContents_ = std::move(rhs.bufferContents_);
        imageContents_ = std::move(rhs.imageContents_);
        return *this;
    }

    unsigned BufferGroup::AddBufferToGroup(vk::BufferUsageFlags usage, size_t size, const std::vector<uint32_t>& queueFamilyIndices)
    {
        deviceBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferDst | usage, memoryProperties_, queueFamilyIndices);
        deviceBuffers_.back().InitializeBuffer(size, false);

        hostBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc, memoryProperties_, queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        bufferContents_.push_back(std::make_pair(0, nullptr));
        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned BufferGroup::AddBufferToGroup(vk::BufferUsageFlags usage, size_t size, const void* data, const std::vector<uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        bufferContents_.back().first = size;
        bufferContents_.back().second = data;
        return idx;
    }

    void MemoryGroup::FinalizeGroup(QueuedDeviceTransfer* transfer)
    {
        vk::MemoryAllocateInfo deviceAllocInfo, hostAllocInfo;
        std::vector<uint32_t> deviceSizes, hostSizes;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            FillBufferAllocationInfo(&hostBuffers_[i], hostAllocInfo, hostSizes);
            FillBufferAllocationInfo(&deviceBuffers_[i], deviceAllocInfo, deviceSizes);
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            FillImageAllocationInfo(&hostImages_[i], hostAllocInfo, hostSizes);
            FillImageAllocationInfo(&deviceImages_[i], deviceAllocInfo, deviceSizes);
        }
        hostMemory_.InitializeMemory(hostAllocInfo);
        deviceMemory_.InitializeMemory(deviceAllocInfo);

        auto deviceOffset = 0U, hostOffset = 0U;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostMemory_.BindToBuffer(hostBuffers_[i], hostOffset);
            deviceMemory_.BindToBuffer(deviceBuffers_[i], deviceOffset);
            if (transfer) {
                hostMemory_.CopyToHostMemory(hostOffset, bufferContents_[i].first, bufferContents_[i].second);
                transfer->AddTransferToQueue(hostBuffers_[i], deviceBuffers_[i]);
            }
            hostOffset += hostSizes[i];
            deviceOffset += deviceSizes[i];
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            hostMemory_.BindToTexture(hostImages_[i], hostOffset);
            deviceMemory_.BindToTexture(deviceImages_[i], deviceOffset);
            if (transfer) {
                hostMemory_.CopyToHostMemory(hostOffset, layout, imageContents_[i].first, imageContents_[i].second)
                transfer->AddTransferToQueue(hostImages_[i], deviceImages_[i]);
            }
            hostOffset += hostSizes[i];
            deviceOffset += deviceSizes[i];
        }
    }

    void MemoryGroup::FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        FillAllocationInfo(memRequirements, buffer->GetDeviceMemory().GetMemoryProperties(), allocInfo, sizes);
    }

    void MemoryGroup::FillImageAllocationInfo(Texture* image, vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getImageMemoryRequirements(image->GetImage());
        FillAllocationInfo(memRequirements, image->GetDeviceMemory().GetMemoryProperties(), allocInfo, sizes);
    }

    void MemoryGroup::FillAllocationInfo(const vk::MemoryRequirements& memRequirements, vk::MemoryPropertyFlags memProperties,
        vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const
    {
        if (allocInfo.allocationSize == 0) allocInfo.memoryTypeIndex =
            DeviceMemory::FindMemoryType(device_, memRequirements.memoryTypeBits, memProperties);
        else if (!DeviceMemory::CheckMemoryType(device_, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, memProperties)) {
            LOG(FATAL) << "BufferGroup memory type (" << allocInfo.memoryTypeIndex << ") does not fit required memory type for buffer ("
                << std::hex << memRequirements.memoryTypeBits << ").";
            throw std::runtime_error("BufferGroup memory type does not fit required memory type for buffer.");
        }

        sizes.push_back(static_cast<unsigned int>(memRequirements.size));
        allocInfo.allocationSize += memRequirements.size;
    }
}}
