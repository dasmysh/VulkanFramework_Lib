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

    unsigned int MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferDst | usage, memoryProperties_, queueFamilyIndices);
        deviceBuffers_.back().InitializeBuffer(size, false);

        hostBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferSrc, memoryProperties_, queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned int MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size,
        const void* data, const std::function<void(void*)>& deleter, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        AddDataToBufferInGroup(idx, 0, size, data, deleter);
        return idx;
    }

    void MemoryGroup::AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize,
        const void * data, const std::function<void(void*)>& deleter)
    {
        BufferContentsDesc bufferContentsDesc;
        bufferContentsDesc.bufferIdx_ = bufferIdx;
        bufferContentsDesc.offset_ = offset;
        bufferContentsDesc.size_ = dataSize;
        bufferContentsDesc.data_ = data;
        bufferContentsDesc.deleter_ = deleter;
        bufferContents_.push_back(bufferContentsDesc);
    }

    unsigned int MemoryGroup::AddTextureToGroup(const TextureDescriptor& desc, const glm::u32vec4& size,
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
        std::uint32_t mipLevel, std::uint32_t arrayLayer, const glm::u32vec3& size,
        const void* data, const std::function<void(void*)>& deleter)
    {
        ImageContentsDesc imgContDesc;
        imgContDesc.imageIdx_ = textureIdx;
        imgContDesc.aspectFlags_ = aspectFlags;
        imgContDesc.mipLevel_ = mipLevel;
        imgContDesc.arrayLayer_ = arrayLayer;
        imgContDesc.size_ = size;
        imgContDesc.data_ = data;
        imgContDesc.deleter_ = deleter;
        imageContents_.push_back(imgContDesc);
    }

    void MemoryGroup::FinalizeGroup()
    {
        vk::MemoryAllocateInfo hostAllocInfo, deviceAllocInfo;
        hostOffsets_.resize(deviceBuffers_.size() + deviceImages_.size());
        deviceOffsets_.resize(deviceBuffers_.size() + deviceImages_.size());
        std::size_t hostOffset = 0U, deviceOffset = 0U;
        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostOffsets_[i] = hostOffset;
            hostOffset += FillBufferAllocationInfo(&hostBuffers_[i], hostAllocInfo);
            deviceOffsets_[i] = deviceOffset;
            deviceOffset += FillBufferAllocationInfo(&deviceBuffers_[i], deviceAllocInfo);
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            hostOffsets_[i + hostBuffers_.size()] = hostOffset;
            hostOffset += FillImageAllocationInfo((i == 0 ? nullptr : &hostImages_[i - 1]), &hostImages_[i],
                hostOffsets_[i + hostBuffers_.size()], hostAllocInfo);
            deviceOffsets_[i + deviceBuffers_.size()] = deviceOffset;
            deviceOffset += FillImageAllocationInfo((i == 0 ? nullptr : &deviceImages_[i - 1]), &deviceImages_[i],
                deviceOffsets_[i + deviceBuffers_.size()], deviceAllocInfo);
        }
        hostMemory_.InitializeMemory(hostAllocInfo);
        deviceMemory_.InitializeMemory(deviceAllocInfo);

        for (auto i = 0U; i < deviceBuffers_.size(); ++i) {
            hostMemory_.BindToBuffer(hostBuffers_[i], hostOffsets_[i]);
            deviceMemory_.BindToBuffer(deviceBuffers_[i], deviceOffsets_[i]);
        }
        for (auto i = 0U; i < deviceImages_.size(); ++i) {
            hostMemory_.BindToTexture(hostImages_[i], hostOffsets_[i + hostBuffers_.size()]);
            deviceMemory_.BindToTexture(deviceImages_[i], deviceOffsets_[i + deviceBuffers_.size()]);
            deviceImages_[i].InitializeImageView();
        }
    }

    void MemoryGroup::TransferData(QueuedDeviceTransfer& transfer)
    {
        for (const auto& contentDesc : bufferContents_) {
            hostMemory_.CopyToHostMemory(
                hostOffsets_[contentDesc.bufferIdx_] + contentDesc.offset_, contentDesc.size_, contentDesc.data_);
            if (contentDesc.deleter_) contentDesc.deleter_(const_cast<void*>(contentDesc.data_));
        }
        for (const auto& contentDesc : imageContents_) {
            vk::ImageSubresource imgSubresource{ contentDesc.aspectFlags_, contentDesc.mipLevel_, contentDesc.arrayLayer_ };
            auto subresourceLayout = device_->GetDevice().getImageSubresourceLayout(hostImages_[contentDesc.imageIdx_].GetImage(), imgSubresource);
            hostMemory_.CopyToHostMemory(hostOffsets_[contentDesc.imageIdx_], glm::u32vec3(0), subresourceLayout, contentDesc.size_, contentDesc.data_);
            if (contentDesc.deleter_) contentDesc.deleter_(const_cast<void*>(contentDesc.data_));
        }

        for (auto i = 0U; i < deviceBuffers_.size(); ++i) transfer.AddTransferToQueue(hostBuffers_[i], deviceBuffers_[i]);
        for (auto i = 0U; i < deviceImages_.size(); ++i) transfer.AddTransferToQueue(hostImages_[i], deviceImages_[i]);

        bufferContents_.clear();
        imageContents_.clear();
    }

    void MemoryGroup::FillUploadBufferCmdBuffer(unsigned int bufferIdx, vk::CommandBuffer cmdBuffer,
        std::size_t offset, std::size_t dataSize)
    {
        vk::BufferCopy copyRegion{ offset, offset, dataSize };
        cmdBuffer.copyBuffer(hostBuffers_[bufferIdx].GetBuffer(), deviceBuffers_[bufferIdx].GetBuffer(), copyRegion);
    }

    std::size_t MemoryGroup::FillBufferAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo) const
    {
        auto memRequirements = device_->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        return FillAllocationInfo(memRequirements, buffer->GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::size_t MemoryGroup::FillImageAllocationInfo(Texture* lastImage, Texture* image, std::size_t& imageOffset, vk::MemoryAllocateInfo& allocInfo) const
    {
        auto memRequirements = device_->GetDevice().getImageMemoryRequirements(image->GetImage());
        std::size_t newOffset;
        if (lastImage == nullptr) newOffset = device_->CalculateBufferImageOffset(*image, imageOffset);
        else newOffset = device_->CalculateImageImageOffset(*lastImage, *lastImage, imageOffset);
        memRequirements.size += newOffset - imageOffset;
        imageOffset = newOffset;
        return FillAllocationInfo(memRequirements, image->GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::size_t MemoryGroup::FillAllocationInfo(const vk::MemoryRequirements& memRequirements,
        vk::MemoryPropertyFlags memProperties, vk::MemoryAllocateInfo& allocInfo) const
    {
        if (allocInfo.allocationSize == 0) allocInfo.memoryTypeIndex =
            DeviceMemory::FindMemoryType(device_, memRequirements.memoryTypeBits, memProperties);
        else if (!DeviceMemory::CheckMemoryType(device_, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, memProperties)) {
            LOG(FATAL) << "MemoryGroup memory type (" << allocInfo.memoryTypeIndex << ") does not fit required memory type for buffer or image ("
                << std::hex << memRequirements.memoryTypeBits << ").";
            throw std::runtime_error("MemoryGroup memory type does not fit required memory type for buffer or image.");
        }

        allocInfo.allocationSize += memRequirements.size;
        return memRequirements.size;
    }
}
