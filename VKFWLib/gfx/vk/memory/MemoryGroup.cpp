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
        hostMemory_{ device, memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent },
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

        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const void* data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        BufferContentsDesc bufferContentsDesc;
        bufferContentsDesc.bufferIdx_ = idx;
        bufferContentsDesc.offset_ = 0;
        bufferContentsDesc.size_ = size;
        bufferContentsDesc.data_ = data;
        bufferContents_.push_back(bufferContentsDesc);
        return idx;
    }

    void MemoryGroup::AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize, const void * data)
    {
        BufferContentsDesc bufferContentsDesc;
        bufferContentsDesc.bufferIdx_ = bufferIdx;
        bufferContentsDesc.offset_ = offset;
        bufferContentsDesc.size_ = dataSize;
        bufferContentsDesc.data_ = data;
        bufferContents_.push_back(bufferContentsDesc);
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
        ImageContentsDesc imgContDesc;
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
        vk::MemoryAllocateInfo hostAllocInfo, deviceAllocInfo;
        std::vector<std::uint32_t> hostOffsets, deviceOffsets;
        hostOffsets.resize(deviceBuffers_.size() + deviceImages_.size());
        deviceOffsets.resize(deviceBuffers_.size() + deviceImages_.size());
        std::uint32_t hostOffset = 0U, deviceOffset = 0U;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostOffsets[i] = hostOffset;
            hostOffset += FillBufferAllocationInfo(&hostBuffers_[i], hostAllocInfo);
            deviceOffsets[i] = deviceOffset;
            deviceOffset += FillBufferAllocationInfo(&deviceBuffers_[i], deviceAllocInfo);
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            hostOffsets[i + hostBuffers_.size()] = hostOffset;
            hostOffset += FillImageAllocationInfo(&hostImages_[i], hostAllocInfo);
            deviceOffsets[i + deviceBuffers_.size()] = deviceOffset;
            deviceOffset += FillImageAllocationInfo(&deviceImages_[i], deviceAllocInfo);
        }
        hostMemory_.InitializeMemory(hostAllocInfo);
        deviceMemory_.InitializeMemory(deviceAllocInfo);

        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostMemory_.BindToBuffer(hostBuffers_[i], hostOffsets[i]);
            deviceMemory_.BindToBuffer(deviceBuffers_[i], deviceOffsets[i]);
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            hostMemory_.BindToTexture(hostImages_[i], hostOffsets[i + hostBuffers_.size()]);
            deviceMemory_.BindToTexture(deviceImages_[i], deviceOffsets[i + deviceBuffers_.size()]);
        }

        if (!transfer) return;

        for (const auto& contentDesc : bufferContents_) hostMemory_.CopyToHostMemory(
            hostOffsets[contentDesc.bufferIdx_] + contentDesc.offset_, contentDesc.size_, contentDesc.data_);
        for (const auto& contentDesc : imageContents_) {
            vk::ImageSubresource imgSubresource{ contentDesc.aspectFlags_, contentDesc.mipLevel_, contentDesc.arrayLayer_ };
            auto subresourceLayout = device_->GetDevice().getImageSubresourceLayout(hostImages_[contentDesc.imageIdx_].GetImage(), imgSubresource);
            hostMemory_.CopyToHostMemory(hostOffsets[contentDesc.imageIdx_], glm::u32vec3(0), subresourceLayout, contentDesc.size_, contentDesc.data_);
        }

        for (auto i = 0U; i < deviceBuffers_.size(); ++i) transfer->AddTransferToQueue(hostBuffers_[i], deviceBuffers_[i]);
        for (auto i = 0U; i < deviceImages_.size(); ++i) transfer->AddTransferToQueue(hostImages_[i], deviceImages_[i]);
    }

    std::uint32_t MemoryGroup::FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        return FillAllocationInfo(memRequirements, buffer->GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::uint32_t MemoryGroup::FillImageAllocationInfo(Texture* image, vk::MemoryAllocateInfo& allocInfo) const
    {
        auto memRequirements = device_->GetDevice().getImageMemoryRequirements(image->GetImage());
        return FillAllocationInfo(memRequirements, image->GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::uint32_t MemoryGroup::FillAllocationInfo(const vk::MemoryRequirements& memRequirements,
        vk::MemoryPropertyFlags memProperties, vk::MemoryAllocateInfo& allocInfo) const
    {
        if (allocInfo.allocationSize == 0) allocInfo.memoryTypeIndex =
            DeviceMemory::FindMemoryType(device_, memRequirements.memoryTypeBits, memProperties);
        else if (!DeviceMemory::CheckMemoryType(device_, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, memProperties)) {
            LOG(FATAL) << "BufferGroup memory type (" << allocInfo.memoryTypeIndex << ") does not fit required memory type for buffer ("
                << std::hex << memRequirements.memoryTypeBits << ").";
            throw std::runtime_error("BufferGroup memory type does not fit required memory type for buffer.");
        }

        allocInfo.allocationSize += memRequirements.size;
        return static_cast<std::uint32_t>(memRequirements.size);
    }
}
