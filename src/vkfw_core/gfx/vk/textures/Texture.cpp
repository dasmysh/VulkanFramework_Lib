/**
 * @file   Texture.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a texture object for Vulkan.
 */

#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/buffers/BufferGroup.h"
#include "gfx/vk/CommandBuffers.h"

namespace vku::gfx {

    TextureDescriptor TextureDescriptor::StagingTextureDesc(std::size_t bytesPP, vk::Format format,
                                                            vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eLinear;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::StagingTextureDesc(const TextureDescriptor& orig)
    {
        TextureDescriptor texDesc = orig;
        texDesc.imageTiling_ = vk::ImageTiling::eLinear;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::SampleOnlyTextureDesc(std::size_t bytesPP, vk::Format format,
                                                               vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eOptimal;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eSampled;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::DepthBufferTextureDesc(std::size_t bytesPP, vk::Format format,
                                                                vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eOptimal;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::eUndefined;
        return texDesc;
    }

    Texture::Texture(const LogicalDevice* device, const TextureDescriptor& desc,
        std::vector<std::uint32_t> queueFamilyIndices) :
        device_{ device },
        imageDeviceMemory_{ device, desc.memoryProperties_ },
        size_{ 0 },
        mipLevels_{ 0 },
        desc_{ desc },
        queueFamilyIndices_{ std::move(queueFamilyIndices) }
    {
        assert(desc_.bytesPP_ > 0);
    }

    Texture::Texture(Texture&& rhs) noexcept :
        device_{ rhs.device_ },
        vkImage_{ std::move(rhs.vkImage_) },
        vkImageView_{ std::move(rhs.vkImageView_) },
        imageDeviceMemory_{ std::move(rhs.imageDeviceMemory_) },
        size_{ rhs.size_ },
        mipLevels_{ rhs.mipLevels_ },
        desc_{ rhs.desc_ },
        queueFamilyIndices_{ std::move(rhs.queueFamilyIndices_) },
        type_{ rhs.type_ },
        viewType_{ rhs.viewType_ }
    {
        rhs.size_ = glm::u32vec4(0);
        rhs.mipLevels_ = 0;
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        this->~Texture();
        device_ = rhs.device_;
        vkImage_ = std::move(rhs.vkImage_);
        vkImageView_ = std::move(rhs.vkImageView_);
        imageDeviceMemory_ = std::move(rhs.imageDeviceMemory_);
        size_ = rhs.size_;
        mipLevels_ = rhs.mipLevels_;
        desc_ = rhs.desc_;
        queueFamilyIndices_ = std::move(rhs.queueFamilyIndices_);
        type_ = rhs.type_;
        viewType_ = rhs.viewType_;
        rhs.size_ = glm::u32vec4(0);
        rhs.mipLevels_ = 0;
        return *this;
    }

    Texture::~Texture() = default;

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
        if (!queueFamilyIndices_.empty()) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(queueFamilyIndices_.size()));
            imgCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }

        vkImage_ = device_->GetDevice().createImageUnique(imgCreateInfo);

        if (initMemory) {
            vk::MemoryRequirements memRequirements = device_->GetDevice().getImageMemoryRequirements(*vkImage_);
            imageDeviceMemory_.InitializeMemory(memRequirements);
            imageDeviceMemory_.BindToTexture(*this, 0);
            InitializeImageView();
        }
    }

    void Texture::InitializeImageView()
    {
        vk::ImageSubresourceRange imgSubresourceRange{ GetValidAspects(), 0, mipLevels_, 0, size_.w };
        vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), *vkImage_, viewType_, desc_.format_, vk::ComponentMapping(), imgSubresourceRange };
        vkImageView_ = device_->GetDevice().createImageViewUnique(imgViewCreateInfo);
    }

    void Texture::TransitionLayout(vk::ImageLayout newLayout, vk::CommandBuffer cmdBuffer)
    {
        vk::ImageSubresourceRange subresourceRange{ GetValidAspects(), 0, mipLevels_, 0, size_.w };
        vk::ImageMemoryBarrier transitionBarrier{ vk::AccessFlags(), vk::AccessFlags(), desc_.imageLayout_,
            newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *vkImage_, subresourceRange };

        transitionBarrier.srcAccessMask = GetAccessFlagsForLayout(desc_.imageLayout_);
        transitionBarrier.dstAccessMask = GetAccessFlagsForLayout(newLayout);

        cmdBuffer.pipelineBarrier(GetStageFlagsForLayout(desc_.imageLayout_), GetStageFlagsForLayout(newLayout),
            vk::DependencyFlags(), nullptr, nullptr, transitionBarrier);
        desc_.imageLayout_ = newLayout;
    }

    vk::UniqueCommandBuffer Texture::TransitionLayout(vk::ImageLayout newLayout,
        std::pair<std::uint32_t, std::uint32_t> transitionQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence)
    {
        if (desc_.imageLayout_ == newLayout) { return vk::UniqueCommandBuffer(); }

        auto transitionCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, transitionQueueIdx.first);
        TransitionLayout(newLayout, *transitionCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(device_, *transitionCmdBuffer, transitionQueueIdx.first, transitionQueueIdx.second,
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
            vk::Extent3D{ size.x / static_cast<std::uint32_t>(desc_.bytesPP_), size.y, size.z } };
        cmdBuffer.copyImage(*vkImage_, desc_.imageLayout_, *dstImage.vkImage_, dstImage.desc_.imageLayout_, copyRegion);
    }

    vk::UniqueCommandBuffer Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, const Texture& dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
        std::pair<std::uint32_t, std::uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        auto transferCmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, copyQueueIdx.first);
        CopyImageAsync(srcMipLevel, srcOffset, dstImage, dstMipLevel, dstOffset, size, *transferCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(device_, *transferCmdBuffer, copyQueueIdx.first, copyQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    vk::UniqueCommandBuffer Texture::CopyImageAsync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), size_, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Texture::CopyImageSync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyImageAsync(dstImage, copyQueueIdx);
        device_->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();
    }

    vk::ImageAspectFlags Texture::GetValidAspects() const
    {
        // TODO: support of the metadata aspect?
        if (desc_.format_ == vk::Format::eD16Unorm || desc_.format_ == vk::Format::eX8D24UnormPack32
            || desc_.format_ == vk::Format::eD32Sfloat) {
            return vk::ImageAspectFlagBits::eDepth;
        }
        if (desc_.format_ == vk::Format::eS8Uint) { return vk::ImageAspectFlagBits::eStencil; }
        if (desc_.format_ == vk::Format::eD16UnormS8Uint || desc_.format_ == vk::Format::eD24UnormS8Uint
            || desc_.format_ == vk::Format::eD32SfloatS8Uint) {
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        }
        return vk::ImageAspectFlagBits::eColor;
    }

    vk::AccessFlags Texture::GetAccessFlagsForLayout(vk::ImageLayout layout)
    {
        switch (layout) {
        case vk::ImageLayout::eUndefined: return vk::AccessFlags();
        case vk::ImageLayout::ePreinitialized: return vk::AccessFlagBits::eHostWrite;
        case vk::ImageLayout::eTransferDstOptimal: return vk::AccessFlagBits::eTransferWrite;
        case vk::ImageLayout::eTransferSrcOptimal: return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eColorAttachmentOptimal: return vk::AccessFlagBits::eColorAttachmentWrite;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::eShaderReadOnlyOptimal: return vk::AccessFlagBits::eShaderRead;
        default: return vk::AccessFlags();
        }
    }

    vk::PipelineStageFlags Texture::GetStageFlagsForLayout(vk::ImageLayout layout)
    {
        switch (layout) {
        case vk::ImageLayout::eUndefined: return vk::PipelineStageFlagBits::eTopOfPipe;
        case vk::ImageLayout::ePreinitialized: return vk::PipelineStageFlagBits::eHost;
        case vk::ImageLayout::eTransferDstOptimal:
        case vk::ImageLayout::eTransferSrcOptimal: return vk::PipelineStageFlagBits::eTransfer;
        case vk::ImageLayout::eColorAttachmentOptimal: return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal: return vk::PipelineStageFlagBits::eEarlyFragmentTests;
        case vk::ImageLayout::eShaderReadOnlyOptimal: return vk::PipelineStageFlagBits::eFragmentShader;
        default: return vk::PipelineStageFlagBits::eTopOfPipe;
        }
    }

}
