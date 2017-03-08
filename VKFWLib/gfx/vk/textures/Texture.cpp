/**
 * @file   Texture.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a texture object for Vulkan.
 */

#include "Texture.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/buffers/BufferGroup.h"

namespace vku { namespace gfx {

    TextureDescriptor TextureDescriptor::StagingTextureDesc(unsigned bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eLinear;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::StagingTextureDesc(const TextureDescriptor orig)
    {
        TextureDescriptor texDesc = orig;
        texDesc.imageTiling_ = vk::ImageTiling::eLinear;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::SampleOnlyTextureDesc(unsigned bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eOptimal;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eSampled;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    Texture::Texture(const LogicalDevice* device, const TextureDescriptor& desc,
        const std::vector<uint32_t>& queueFamilyIndices) :
        device_{ device },
        imageDeviceMemory_{ device, desc.memoryProperties_ },
        size_{ 0 },
        mipLevels_{ 0 },
        desc_{ desc },
        queueFamilyIndices_{ queueFamilyIndices }
    {
        assert(desc_.bytesPP_ > 0);
    }

    void Texture::InitializeImage(const glm::u32vec4& size, uint32_t mipLevels, bool initMemory)
    {
        assert(size.x > 0);
        assert(size.y > 0);
        assert(size.z > 0);
        assert(size.w > 0);
        assert(mipLevels > 0);
        size_ = glm::u32vec4(size.x * desc_.bytesPP_, size.y, size.z, size.w);
        mipLevels_ = mipLevels;
        vk::ImageType type = vk::ImageType::e3D;
        if (size.z == 1 && size.y == 1) type = vk::ImageType::e1D;
        else if (size.z == 1) type = vk::ImageType::e2D;

        vk::ImageCreateInfo imgCreateInfo{ desc_.createFlags_, type, desc_.format_,
            vk::Extent3D(size.x, size_.y, size_.z),
            mipLevels, size_.w, desc_.samples_, desc_.imageTiling_,
            desc_.imageUsage_, desc_.sharingMode_, 0, nullptr, desc_.imageLayout_ };
        if (queueFamilyIndices_.size() > 0) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices_.size()));
            imgCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }

        vkImage_ = device_->GetDevice().createImage(imgCreateInfo);

        if (initMemory) {
            vk::MemoryRequirements memRequirements = device_->GetDevice().getImageMemoryRequirements(vkImage_);
            imageDeviceMemory_.InitializeMemory(memRequirements);
            imageDeviceMemory_.BindToTexture(*this, 0);
        }
    }

    void Texture::TransitionLayout(vk::ImageLayout newLayout) const
    {
        if (desc_.imageLayout_ == newLayout) return;
        // TODO: implement [3/8/2017 mysh]
    }

    vk::CommandBuffer Texture::CopyImageAsync(uint32_t srcMipLevel, const glm::u32vec4& srcOffset, const Texture& dstImage,
        uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
        std::pair<uint32_t, uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        assert(desc_.imageUsage_ & vk::ImageUsageFlagBits::eTransferSrc);
        assert(dstImage.desc_.imageUsage_ & vk::ImageUsageFlagBits::eTransferDst);
        assert(srcOffset.x + size.x <= size_.x);
        assert(srcOffset.y + size.y <= size_.y);
        assert(srcOffset.z + size.z <= size_.z);
        assert(srcOffset.w + size.w <= size_.w);
        assert(dstOffset.x + size.x <= dstImage.size_.x);
        assert(dstOffset.y + size.y <= dstImage.size_.y);
        assert(dstOffset.z + size.z <= dstImage.size_.z);
        assert(dstOffset.w + size.w <= dstImage.size_.w);
        assert(srcMipLevel < mipLevels_);
        assert(dstMipLevel < dstImage.mipLevels_);

        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device_->GetCommandPool(copyQueueIdx.first) , vk::CommandBufferLevel::ePrimary, 1 };
        auto transferCmdBuffer = device_->GetDevice().allocateCommandBuffers(cmdBufferallocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        transferCmdBuffer.begin(beginInfo);
        vk::ImageSubresourceLayers subresourceLayersSrc{ GetValidAspects(), srcMipLevel, srcOffset.w, size.w };
        vk::ImageSubresourceLayers subresourceLayersDst{ dstImage.GetValidAspects(), dstMipLevel, dstOffset.w, size.w };
        vk::ImageCopy copyRegion{ subresourceLayersSrc,
            vk::Offset3D{ static_cast<int32_t>(srcOffset.x), static_cast<int32_t>(srcOffset.y), static_cast<int32_t>(srcOffset.z) },
            subresourceLayersDst,
            vk::Offset3D{ static_cast<int32_t>(dstOffset.x), static_cast<int32_t>(dstOffset.y), static_cast<int32_t>(dstOffset.z) },
            vk::Extent3D{ size.x / desc_.bytesPP_, size.y, size.z } };
        transferCmdBuffer.copyImage(vkImage_, desc_.imageLayout_, dstImage.vkImage_, dstImage.desc_.imageLayout_, copyRegion);
        transferCmdBuffer.end();

        vk::SubmitInfo submitInfo{ static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, 1, &transferCmdBuffer, static_cast<uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).submit(submitInfo, fence);

        return transferCmdBuffer;
    }

    vk::CommandBuffer Texture::CopyImageAsync(const Texture& dstImage, std::pair<uint32_t, uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), size_, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Texture::CopyImageSync(const Texture& dstImage, std::pair<uint32_t, uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyImageAsync(dstImage, copyQueueIdx);
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();

        device_->GetDevice().freeCommandBuffers(device_->GetCommandPool(copyQueueIdx.first), cmdBuffer);
    }

    vk::ImageAspectFlags Texture::GetValidAspects() const
    {
        // TODO: support of the metadata aspect?
        if (desc_.format_ == vk::Format::eD16Unorm || desc_.format_ == vk::Format::eX8D24UnormPack32
            || desc_.format_ == vk::Format::eD32Sfloat) return vk::ImageAspectFlagBits::eDepth;
        if (desc_.format_ == vk::Format::eS8Uint) return vk::ImageAspectFlagBits::eStencil;
        if (desc_.format_ == vk::Format::eD16UnormS8Uint || desc_.format_ == vk::Format::eD24UnormS8Uint
            || desc_.format_ == vk::Format::eD32SfloatS8Uint) return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        return vk::ImageAspectFlagBits::eColor;
    }
}}
