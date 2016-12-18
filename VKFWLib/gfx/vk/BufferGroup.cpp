/**
 * @file   BufferGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.27
 *
 * @brief  Implementation of the BufferGroup class.
 */

#include "BufferGroup.h"
#include "Buffer.h"
// ReSharper disable CppUnusedIncludeDirective
#include "DeviceBuffer.h"
#include "HostBuffer.h"
// ReSharper restore CppUnusedIncludeDirective
#include "QueuedDeviceTransfer.h"

namespace vku { namespace gfx {

    BufferGroup::BufferGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags) :
        device_{ device },
        memoryProperties_{ memoryFlags }
    {
    }


    BufferGroup::~BufferGroup()
    {
        if (hostBufferMemory_) device_->GetDevice().freeMemory(hostBufferMemory_);
        hostBufferMemory_ = vk::DeviceMemory();
        if (deviceBufferMemory_) device_->GetDevice().freeMemory(deviceBufferMemory_);
        deviceBufferMemory_ = vk::DeviceMemory();
    }

    BufferGroup::BufferGroup(BufferGroup&& rhs) noexcept :
        device_{ rhs.device_ },
        deviceBufferMemory_{ rhs.deviceBufferMemory_ },
        hostBufferMemory_{ rhs.hostBufferMemory_ },
        deviceBuffers_{ std::move(rhs.deviceBuffers_) },
        hostBuffers_{ std::move(rhs.hostBuffers_) },
        memoryProperties_{ rhs.memoryProperties_ },
        bufferContents_{ std::move(rhs.bufferContents_) }
    {
        rhs.deviceBufferMemory_ = vk::DeviceMemory();
        rhs.hostBufferMemory_ = vk::DeviceMemory();
    }

    BufferGroup& BufferGroup::operator=(BufferGroup&& rhs) noexcept
    {
        this->~BufferGroup();
        device_ = rhs.device_;
        deviceBufferMemory_ = rhs.deviceBufferMemory_;
        hostBufferMemory_ = rhs.hostBufferMemory_;
        deviceBuffers_ = std::move(rhs.deviceBuffers_);
        hostBuffers_ = std::move(rhs.hostBuffers_);
        memoryProperties_ = rhs.memoryProperties_;
        bufferContents_ = std::move(rhs.bufferContents_);
        rhs.deviceBufferMemory_ = vk::DeviceMemory();
        rhs.hostBufferMemory_ = vk::DeviceMemory();
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

    void BufferGroup::FinalizeGroup(QueuedDeviceTransfer* transfer)
    {
        vk::MemoryAllocateInfo deviceAllocInfo, hostAllocInfo;
        std::vector<uint32_t> deviceSizes, hostSizes;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            FillAllocationInfo(&hostBuffers_[i], hostAllocInfo, hostSizes);
            FillAllocationInfo(&deviceBuffers_[i], deviceAllocInfo, deviceSizes);
        }
        hostBufferMemory_ = device_->GetDevice().allocateMemory(hostAllocInfo);
        deviceBufferMemory_ = device_->GetDevice().allocateMemory(deviceAllocInfo);

        auto deviceOffset = 0U, hostOffset = 0U;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            device_->GetDevice().bindBufferMemory(*hostBuffers_[i].GetBuffer(), hostBufferMemory_, hostOffset);
            device_->GetDevice().bindBufferMemory(*deviceBuffers_[i].GetBuffer(), deviceBufferMemory_, deviceOffset);
            if (transfer) {
                auto deviceMem = device_->GetDevice().mapMemory(hostBufferMemory_, hostOffset, bufferContents_[i].first, vk::MemoryMapFlags());
                memcpy(deviceMem, bufferContents_[i].second, bufferContents_[i].first);
                device_->GetDevice().unmapMemory(hostBufferMemory_);

                // hostBuffers_[i].UploadData(0, bufferContents_[i].first, bufferContents_[i].second);
                transfer->AddTransferToQueue(hostBuffers_[i], deviceBuffers_[i]);
            }
            hostOffset += hostSizes[i];
            deviceOffset += deviceSizes[i];

        }
    }

    void BufferGroup::FillAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(*buffer->GetBuffer());
        if (allocInfo.allocationSize == 0) allocInfo.memoryTypeIndex = FindMemoryType(device_, memRequirements.memoryTypeBits, buffer->GetMemoryProperties());
        else if (!CheckMemoryType(device_, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, buffer->GetMemoryProperties())) {
            LOG(FATAL) << "BufferGroup memory type (" << allocInfo.memoryTypeIndex << ") does not fit required memory type for buffer ("
                << std::hex << memRequirements.memoryTypeBits << ").";
            throw std::runtime_error("BufferGroup memory type does not fit required memory type for buffer.");
        }

        sizes.push_back(static_cast<unsigned int>(memRequirements.size));
        allocInfo.allocationSize += memRequirements.size;
    }

    uint32_t BufferGroup::FindMemoryType(const LogicalDevice* device, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (CheckMemoryType(memProperties, i, typeFilter, properties)) return i;
        }

        LOG(FATAL) << "Failed to find suitable memory type (" << vk::to_string(properties) << ").";
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    bool BufferGroup::CheckMemoryType(const LogicalDevice* device, uint32_t typeToCheck, uint32_t typeFilter,
        vk::MemoryPropertyFlags properties)
    {
        auto memProperties = device->GetPhysicalDevice().getMemoryProperties();
        return CheckMemoryType(memProperties, typeToCheck, typeFilter, properties);
    }

    bool BufferGroup::CheckMemoryType(const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeToCheck,
        uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        if ((typeFilter & (1 << typeToCheck)) && (memProperties.memoryTypes[typeToCheck].propertyFlags & properties) == properties) return true;
        return false;
    }
}}
