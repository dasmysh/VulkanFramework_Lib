/**
 * @file   MemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of the MemoryGroup class.
 */

#include "MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vku::gfx {

    MemoryGroup::MemoryGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags) :
        DeviceMemoryGroup(device, memoryFlags),
        hostMemory_{ device, memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent }
    {
    }


    MemoryGroup::~MemoryGroup() = default;

    MemoryGroup::MemoryGroup(MemoryGroup&& rhs) noexcept :
        DeviceMemoryGroup(std::move(rhs)),
        hostMemory_{ std::move(rhs.hostMemory_) },
        hostBuffers_{ std::move(rhs.hostBuffers_) },
        hostImages_{ std::move(rhs.hostImages_) },
        hostOffsets_{ std::move(rhs.hostOffsets_) },
        bufferContents_{ std::move(rhs.bufferContents_) },
        imageContents_{ std::move(rhs.imageContents_) }
    {
    }

    MemoryGroup& MemoryGroup::operator=(MemoryGroup&& rhs) noexcept
    {
        this->~MemoryGroup();
        DeviceMemoryGroup* tDMG = this;
        *tDMG = static_cast<DeviceMemoryGroup&&>(std::move(rhs));
        hostMemory_ = std::move(rhs.hostMemory_);
        hostBuffers_ = std::move(rhs.hostBuffers_);
        hostImages_ = std::move(rhs.hostImages_);
        hostOffsets_ = std::move(rhs.hostOffsets_);
        bufferContents_ = std::move(rhs.bufferContents_);
        imageContents_ = std::move(rhs.imageContents_);
        return *this;
    }

    unsigned int MemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = DeviceMemoryGroup::AddBufferToGroup(usage, size, queueFamilyIndices);

        hostBuffers_.emplace_back(GetDevice(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlags(), queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        return idx;
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
        auto idx = DeviceMemoryGroup::AddTextureToGroup(desc, size, mipLevels, queueFamilyIndices);

        TextureDescriptor stagingTexDesc{ desc, vk::ImageUsageFlagBits::eTransferSrc };
        stagingTexDesc.imageTiling_ = vk::ImageTiling::eLinear;
        hostImages_.emplace_back(GetDevice(), stagingTexDesc, queueFamilyIndices);
        hostImages_.back().InitializeImage(size, mipLevels, false);

        return idx;
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

    void MemoryGroup::FinalizeDeviceGroup()
    {
        InitializeHostMemory(GetDevice(), hostOffsets_, hostBuffers_, hostImages_, hostMemory_);
        BindHostObjects(hostOffsets_, hostBuffers_, hostImages_, hostMemory_);
        DeviceMemoryGroup::FinalizeDeviceGroup();
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
            auto subresourceLayout = GetDevice()->GetDevice().getImageSubresourceLayout(hostImages_[contentDesc.imageIdx_].GetImage(), imgSubresource);
            hostMemory_.CopyToHostMemory(hostOffsets_[contentDesc.imageIdx_ + hostBuffers_.size()], glm::u32vec3(0),
                subresourceLayout, contentDesc.size_, contentDesc.data_);
            if (contentDesc.deleter_) contentDesc.deleter_(const_cast<void*>(contentDesc.data_));
        }

        for (auto i = 0U; i < hostBuffers_.size(); ++i) transfer.AddTransferToQueue(hostBuffers_[i], *GetBuffer(i));
        for (auto i = 0U; i < hostImages_.size(); ++i) transfer.AddTransferToQueue(hostImages_[i], *GetTexture(i));

        bufferContents_.clear();
        imageContents_.clear();
    }

    void MemoryGroup::RemoveHostMemory()
    {
        hostBuffers_.clear();
        hostImages_.clear();
        hostMemory_.~DeviceMemory();
        hostOffsets_.clear();
    }

    void MemoryGroup::FillUploadBufferCmdBuffer(unsigned int bufferIdx, vk::CommandBuffer cmdBuffer,
        std::size_t offset, std::size_t dataSize)
    {
        vk::BufferCopy copyRegion{ offset, offset, dataSize };
        cmdBuffer.copyBuffer(hostBuffers_[bufferIdx].GetBuffer(), GetBuffer(bufferIdx)->GetBuffer(), copyRegion);
    }
}
