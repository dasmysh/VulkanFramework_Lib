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
#include "gfx/vk/CommandBuffers.h"

namespace vku::gfx {

    TextureDescriptor TextureDescriptor::StagingTextureDesc(unsigned int bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
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

    TextureDescriptor TextureDescriptor::SampleOnlyTextureDesc(unsigned int bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eOptimal;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eSampled;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    bool TextureDescriptor::IsFormatSupported(vk::PhysicalDevice physicalDevice)
    {
        auto formatProperties = physicalDevice.getFormatProperties(format_);
        if (imageTiling_ == vk::ImageTiling::eLinear) return formatProperties.linearTilingFeatures != vk::FormatFeatureFlags();
        if (imageTiling_ == vk::ImageTiling::eOptimal) return formatProperties.optimalTilingFeatures != vk::FormatFeatureFlags();
        return false;
    }

    Texture::Texture(const LogicalDevice* device, const TextureDescriptor& desc,
        const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        imageDeviceMemory_{ device, desc.memoryProperties_ },
        size_{ 0 },
        mipLevels_{ 0 },
        desc_{ desc },
        queueFamilyIndices_{ queueFamilyIndices }
    {
        assert(desc_.bytesPP_ > 0);
    }

    Texture::Texture(Texture&& rhs) noexcept :
        device_{ rhs.device_ },
        vkImage_{ rhs.vkImage_ },
        vkImageView_{ rhs.vkImageView_ },
        imageDeviceMemory_{ std::move(rhs.imageDeviceMemory_) },
        size_{ rhs.size_ },
        mipLevels_{ rhs.mipLevels_ },
        desc_{ rhs.desc_ },
        queueFamilyIndices_{ std::move(rhs.queueFamilyIndices_) }
    {
        rhs.vkImage_ = vk::Image();
        rhs.vkImageView_ = vk::ImageView();
        rhs.size_ = glm::u32vec4(0);
        rhs.mipLevels_ = 0;
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        this->~Texture();
        device_ = rhs.device_;
        vkImage_ = rhs.vkImage_;
        vkImageView_ = rhs.vkImageView_;
        imageDeviceMemory_ = std::move(rhs.imageDeviceMemory_);
        size_ = rhs.size_;
        mipLevels_ = rhs.mipLevels_;
        desc_ = rhs.desc_;
        queueFamilyIndices_ = std::move(rhs.queueFamilyIndices_);
        rhs.vkImage_ = vk::Image();
        rhs.vkImageView_ = vk::ImageView();
        rhs.size_ = glm::u32vec4(0);
        rhs.mipLevels_ = 0;
        return *this;
    }

    Texture::~Texture()
    {
        if (vkImage_) device_->GetDevice().destroyImage(vkImage_);
        vkImage_ = vk::Image();
        if (vkImageView_) device_->GetDevice().destroyImageView(vkImageView_);
        vkImageView_ = vk::ImageView();
    }

    void Texture::InitializeImage(const glm::u32vec4& size, std::uint32_t mipLevels, bool initMemory)
    {
        assert(size.x > 0);
        assert(size.y > 0);
        assert(size.z > 0);
        assert(size.w > 0);
        assert(mipLevels > 0);

        this->~Texture();

        size_ = glm::u32vec4(size.x * desc_.bytesPP_, size.y, size.z, size.w);
        mipLevels_ = mipLevels;
        if (size.z == 1 && size.y == 1 && size.w == 1) { type_ = vk::ImageType::e1D; viewType_ = vk::ImageViewType::e1D; }
        else if (size.z == 1 && size.y == 1) { type_ = vk::ImageType::e1D; viewType_ = vk::ImageViewType::e1DArray; }
        else if (size.z == 1 && size.w == 1) { type_ = vk::ImageType::e2D; viewType_ = vk::ImageViewType::e2D; }
        else if (size.z == 1) { type_ = vk::ImageType::e2D; viewType_ = vk::ImageViewType::e2DArray; }
        // else if (size.z == 6 && size.w == 1) { type_ = vk::ImageType::e2D; viewType_ = vk::ImageViewType::eCubeMap; }
        // TODO: Add cube map support later. [3/17/2017 Sebastian Maisch]

        vk::ImageCreateInfo imgCreateInfo{ desc_.createFlags_, type_, desc_.format_,
            vk::Extent3D(size.x, size_.y, size_.z),
            mipLevels, size_.w, desc_.samples_, desc_.imageTiling_,
            desc_.imageUsage_, desc_.sharingMode_, 0, nullptr, desc_.imageLayout_ };
        if (queueFamilyIndices_.size() > 0) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(queueFamilyIndices_.size()));
            imgCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }

        vkImage_ = device_->GetDevice().createImage(imgCreateInfo);

        if (initMemory) {
            vk::MemoryRequirements memRequirements = device_->GetDevice().getImageMemoryRequirements(vkImage_);
            imageDeviceMemory_.InitializeMemory(memRequirements);
            imageDeviceMemory_.BindToTexture(*this, 0);
            InitializeImageView();
        }
    }

    void Texture::InitializeImageView()
    {
        vk::ImageSubresourceRange imgSubresourceRange{ GetValidAspects(), 0, mipLevels_, 0, size_.w };
        vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), vkImage_, viewType_, desc_.format_, vk::ComponentMapping(), imgSubresourceRange };
        vkImageView_ = device_->GetDevice().createImageView(imgViewCreateInfo);
    }

    void Texture::TransitionLayout(vk::ImageLayout newLayout, vk::CommandBuffer cmdBuffer)
    {
        vk::ImageSubresourceRange subresourceRange{ GetValidAspects(), 0, mipLevels_, 0, size_.w };
        vk::ImageMemoryBarrier transitionBarrier{ vk::AccessFlags(), vk::AccessFlags(), desc_.imageLayout_,
            newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, vkImage_, subresourceRange };

        if (desc_.imageLayout_ == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
            transitionBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
            transitionBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        }
        else if (desc_.imageLayout_ == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            transitionBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
            transitionBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        }
        else if (desc_.imageLayout_ == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            transitionBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            transitionBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
            vk::DependencyFlags(), nullptr, nullptr, transitionBarrier);
        desc_.imageLayout_ = newLayout;
    }

    vk::CommandBuffer Texture::TransitionLayout(vk::ImageLayout newLayout,
        std::pair<std::uint32_t, std::uint32_t> transitionQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence)
    {
        if (desc_.imageLayout_ == newLayout) return vk::CommandBuffer();

        auto transitionCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, transitionQueueIdx.first);
        TransitionLayout(newLayout, transitionCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(device_, transitionCmdBuffer, transitionQueueIdx.first, transitionQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);
        return transitionCmdBuffer;
    }

    void Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4 & srcOffset, const Texture & dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4 & dstOffset, const glm::u32vec4 & size, vk::CommandBuffer cmdBuffer) const
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

        vk::ImageSubresourceLayers subresourceLayersSrc{ GetValidAspects(), srcMipLevel, srcOffset.w, size.w };
        vk::ImageSubresourceLayers subresourceLayersDst{ dstImage.GetValidAspects(), dstMipLevel, dstOffset.w, size.w };
        vk::ImageCopy copyRegion{ subresourceLayersSrc,
            vk::Offset3D{ static_cast<std::int32_t>(srcOffset.x), static_cast<std::int32_t>(srcOffset.y), static_cast<std::int32_t>(srcOffset.z) },
            subresourceLayersDst,
            vk::Offset3D{ static_cast<std::int32_t>(dstOffset.x), static_cast<std::int32_t>(dstOffset.y), static_cast<std::int32_t>(dstOffset.z) },
            vk::Extent3D{ size.x / desc_.bytesPP_, size.y, size.z } };
        cmdBuffer.copyImage(vkImage_, desc_.imageLayout_, dstImage.vkImage_, dstImage.desc_.imageLayout_, copyRegion);
    }

    vk::CommandBuffer Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, const Texture& dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
        std::pair<std::uint32_t, std::uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        auto transferCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, copyQueueIdx.first);
        /*vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device_->GetCommandPool(copyQueueIdx.first) , vk::CommandBufferLevel::ePrimary, 1 };
        auto transferCmdBuffer = device_->GetDevice().allocateCommandBuffers(cmdBufferallocInfo)[0];*/

        CopyImageAsync(srcMipLevel, srcOffset, dstImage, dstMipLevel, dstOffset, size, transferCmdBuffer);
        /*vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        transferCmdBuffer.begin(beginInfo);*/
        /*vk::ImageSubresourceLayers subresourceLayersSrc{ GetValidAspects(), srcMipLevel, srcOffset.w, size.w };
        vk::ImageSubresourceLayers subresourceLayersDst{ dstImage.GetValidAspects(), dstMipLevel, dstOffset.w, size.w };
        vk::ImageCopy copyRegion{ subresourceLayersSrc,
            vk::Offset3D{ static_cast<std::int32_t>(srcOffset.x), static_cast<std::int32_t>(srcOffset.y), static_cast<std::int32_t>(srcOffset.z) },
            subresourceLayersDst,
            vk::Offset3D{ static_cast<std::int32_t>(dstOffset.x), static_cast<std::int32_t>(dstOffset.y), static_cast<std::int32_t>(dstOffset.z) },
            vk::Extent3D{ size.x / desc_.bytesPP_, size.y, size.z } };
        transferCmdBuffer.copyImage(vkImage_, desc_.imageLayout_, dstImage.vkImage_, dstImage.desc_.imageLayout_, copyRegion);*/
        CommandBuffers::endSingleTimeSubmit(device_, transferCmdBuffer, copyQueueIdx.first, copyQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);
        /*transferCmdBuffer.end();

        vk::SubmitInfo submitInfo{ static_cast<std::uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, 1, &transferCmdBuffer, static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).submit(submitInfo, fence);*/

        return transferCmdBuffer;
    }

    vk::CommandBuffer Texture::CopyImageAsync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), size_, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Texture::CopyImageSync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const
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
}
