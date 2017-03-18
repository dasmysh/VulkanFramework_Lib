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

namespace vku::gfx {

    MemoryGroup::MemoryGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags) :
        device_{ device },
        deviceMemory_{ device, memoryFlags },
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

    unsigned MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferDst | usage, memoryProperties_, queueFamilyIndices);
        deviceBuffers_.back().InitializeBuffer(size, false);

        hostBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc, memoryProperties_, queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        bufferContents_.push_back(std::make_pair(0, nullptr));
        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const void* data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        bufferContents_.back().first = size;
        bufferContents_.back().second = data;
        return idx;
    }

    unsigned MemoryGroup::AddTextureToGroup(const TextureDescriptor& desc, const glm::u32vec4& size,
        std::uint32_t mipLevels, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceImages_.emplace_back(device_, TextureDescriptor(desc, vk::ImageUsageFlagBits::eTransferDst), queueFamilyIndices);
        deviceImages_.back().InitializeImage(size, mipLevels, false);

        TextureDescriptor stagingTexDesc{ desc, vk::ImageUsageFlagBits::eTransferSrc };
        stagingTexDesc.imageTiling_ = vk::ImageTiling::eLinear;
        hostImages_.emplace_back(device_, stagingTexDesc, queueFamilyIndices);
        hostImages_.back().InitializeImage(size, mipLevels, false);

        return static_cast<unsigned int>(deviceImages_.size() - 1);
    }

    void MemoryGroup::AddDataToTextureInGroup(unsigned int textureIdx, vk::ImageAspectFlags aspectFlags,
        std::uint32_t mipLevel, std::uint32_t arrayLayer, const glm::u32vec3& size, const void* data)
    {
        ImageContensDesc imgContDesc;
        imgContDesc.imageIdx_ = textureIdx;
        imgContDesc.aspectFlags_ = aspectFlags;
        imgContDesc.mipLevel_ = mipLevel;
        imgContDesc.arrayLayer_ = arrayLayer;
        imgContDesc.size_ = size;
        imgContDesc.data_ = data;
        imageContents_.push_back(imgContDesc);
    }

    void MemoryGroup::FinalizeGroup(QueuedDeviceTransfer* transfer)
    {
        vk::MemoryAllocateInfo deviceAllocInfo, hostAllocInfo;
        std::vector<std::uint32_t> deviceSizes, hostSizes;
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

        std::vector<std::uint32_t> deviceImageOffsets, hostImageOffsets;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostImageOffsets[i] = hostOffset;
            deviceImageOffsets[i] = deviceOffset;
            hostOffset += hostSizes[i + hostBuffers_.size()];
            deviceOffset += deviceSizes[i + deviceBuffers_.size()];
        }
        for (const auto& contentDesc : imageContents_) {
            hostMemory_.BindToTexture(hostImages_[contentDesc.imageIdx_], hostImageOffsets[contentDesc.imageIdx_]);
            deviceMemory_.BindToTexture(deviceImages_[contentDesc.imageIdx_], deviceImageOffsets[contentDesc.imageIdx_]);
            if (transfer) {
                vk::ImageSubresource imgSubresource{ contentDesc.aspectFlags_, contentDesc.mipLevel_, contentDesc.arrayLayer_ };
                auto subresourceLayout = device_->GetDevice().getImageSubresourceLayout(hostImages_[contentDesc.imageIdx_].GetImage(), imgSubresource);
                hostMemory_.CopyToHostMemory(hostOffset, glm::u32vec3(0), subresourceLayout, contentDesc.size_, contentDesc.data_);
                transfer->AddTransferToQueue(hostImages_[contentDesc.imageIdx_], deviceImages_[contentDesc.imageIdx_]);
            }
        }
    }

    void MemoryGroup::FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        FillAllocationInfo(memRequirements, buffer->GetDeviceMemory().GetMemoryProperties(), allocInfo, sizes);
    }

    void MemoryGroup::FillImageAllocationInfo(Texture* image, vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const
    {
        auto memRequirements = device_->GetDevice().getImageMemoryRequirements(image->GetImage());
        FillAllocationInfo(memRequirements, image->GetDeviceMemory().GetMemoryProperties(), allocInfo, sizes);
    }

    void MemoryGroup::FillAllocationInfo(const vk::MemoryRequirements& memRequirements, vk::MemoryPropertyFlags memProperties,
        vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const
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
}
