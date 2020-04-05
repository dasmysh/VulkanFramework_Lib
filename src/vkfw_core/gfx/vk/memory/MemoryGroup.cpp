/**
 * @file   MemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of the MemoryGroup class.
 */

#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vkfw_core::gfx {

    template<class T> struct always_false : std::false_type
    {
    };

    MemoryGroup::MemoryGroup(const LogicalDevice* device, const vk::MemoryPropertyFlags& memoryFlags) :
        DeviceMemoryGroup(device, memoryFlags),
        hostMemory_{ device, memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent }
    {
    }

    MemoryGroup::~MemoryGroup() = default;
    MemoryGroup::MemoryGroup(MemoryGroup&& rhs) noexcept = default;
    MemoryGroup& MemoryGroup::operator=(MemoryGroup&& rhs) noexcept = default;

    unsigned int MemoryGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = DeviceMemoryGroup::AddBufferToGroup(usage, size, queueFamilyIndices);

        hostBuffers_.emplace_back(GetDevice(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlags(), queueFamilyIndices);
        hostBuffers_.back().InitializeBuffer(size, false);

        return idx;
    }

    unsigned int MemoryGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size,
                                               std::variant<void*, const void*> data,
                                               const std::function<void(void*)>& deleter,
                                               const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        AddDataToBufferInGroup(idx, 0, size, data, deleter);
        return idx;
    }

    void MemoryGroup::AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize,
                                             std::variant<void*, const void*> data,
                                             const std::function<void(void*)>& deleter)
    {
        assert(std::holds_alternative<void*>(data) || !deleter);
        BufferContentsDesc bufferContentsDesc;
        bufferContentsDesc.bufferIdx_ = bufferIdx;
        bufferContentsDesc.offset_ = offset;
        bufferContentsDesc.size_ = dataSize;
        if (deleter) {

            bufferContentsDesc.data_ = data;
            bufferContentsDesc.deleter_ = deleter;
        } else {
            bufferContentsDesc.data_ = std::visit(
                [](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, void*>) {
                        return const_cast<const void*>(arg);
                    } else if constexpr (std::is_same_v<T, const void*>) {
                        return arg;
                    }
                    else static_assert(always_false<T>::value, "non-exhaustive visitor!");
                },
                data);
            bufferContentsDesc.deleter_ = nullptr;
        }
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

    void MemoryGroup::AddDataToTextureInGroup(unsigned int textureIdx, const vk::ImageAspectFlags& aspectFlags,
                                              std::uint32_t mipLevel, std::uint32_t arrayLayer,
                                              const glm::u32vec3& size, std::variant<void*, const void*> data,
                                              const std::function<void(void*)>& deleter)
    {
        assert((deleter && std::holds_alternative<void*>(data))
               || (!deleter && std::holds_alternative<const void*>(data)));
        ImageContentsDesc imgContDesc;
        imgContDesc.imageIdx_ = textureIdx;
        imgContDesc.aspectFlags_ = aspectFlags;
        imgContDesc.mipLevel_ = mipLevel;
        imgContDesc.arrayLayer_ = arrayLayer;
        imgContDesc.size_ = size;
        if (deleter) {

            imgContDesc.data_ = data;
            imgContDesc.deleter_ = deleter;
        } else {
            imgContDesc.data_ = std::visit(
                [](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, void*>) {
                        return const_cast<const void*>(arg);
                    } else if constexpr (std::is_same_v<T, const void*>) {
                        return arg;
                    } else
                        static_assert(always_false<T>::value, "non-exhaustive visitor!");
                },
                data);
            imgContDesc.deleter_ = nullptr;
        }
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
                hostOffsets_[contentDesc.bufferIdx_] + contentDesc.offset_, contentDesc.size_,
                                         contentDesc.deleter_ ? std::get<void*>(contentDesc.data_)
                                                              : std::get<const void*>(contentDesc.data_));
            if (contentDesc.deleter_) { contentDesc.deleter_( std::get<void*>(contentDesc.data_)); }
        }
        for (const auto& contentDesc : imageContents_) {
            vk::ImageSubresource imgSubresource{ contentDesc.aspectFlags_, contentDesc.mipLevel_, contentDesc.arrayLayer_ };
            auto subresourceLayout = GetDevice()->GetDevice().getImageSubresourceLayout(hostImages_[contentDesc.imageIdx_].GetImage(), imgSubresource);
            hostMemory_.CopyToHostMemory(hostOffsets_[contentDesc.imageIdx_ + hostBuffers_.size()], glm::u32vec3(0),
                                         subresourceLayout, contentDesc.size_,
                                         contentDesc.deleter_ ? std::get<void*>(contentDesc.data_)
                                                              : std::get<const void*>(contentDesc.data_));
            if (contentDesc.deleter_) { contentDesc.deleter_(std::get<void*>(contentDesc.data_)); }
        }

        for (auto i = 0U; i < hostBuffers_.size(); ++i) { transfer.AddTransferToQueue(hostBuffers_[i], *GetBuffer(i)); }
        for (auto i = 0U; i < hostImages_.size(); ++i) { transfer.AddTransferToQueue(hostImages_[i], *GetTexture(i)); }

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
