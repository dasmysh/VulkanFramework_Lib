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

namespace vkfw_core::gfx {

    TextureDescriptor TextureDescriptor::StagingTextureDesc(std::size_t bytesPP, vk::Format format,
                                                            vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.m_createFlags = vk::ImageCreateFlags();
        texDesc.m_imageTiling = vk::ImageTiling::eLinear;
        texDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.m_sharingMode = vk::SharingMode::eExclusive;
        texDesc.m_imageLayout = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::StagingTextureDesc(const TextureDescriptor& orig)
    {
        TextureDescriptor texDesc = orig;
        texDesc.m_imageTiling = vk::ImageTiling::eLinear;
        texDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.m_sharingMode = vk::SharingMode::eExclusive;
        texDesc.m_imageLayout = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::SampleOnlyTextureDesc(std::size_t bytesPP, vk::Format format,
                                                               vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.m_createFlags = vk::ImageCreateFlags();
        texDesc.m_imageTiling = vk::ImageTiling::eOptimal;
        texDesc.m_imageUsage = vk::ImageUsageFlagBits::eSampled;
        texDesc.m_sharingMode = vk::SharingMode::eExclusive;
        texDesc.m_imageLayout = vk::ImageLayout::ePreinitialized;
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::DepthBufferTextureDesc(std::size_t bytesPP, vk::Format format,
                                                                vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.m_createFlags = vk::ImageCreateFlags();
        texDesc.m_imageTiling = vk::ImageTiling::eOptimal;
        texDesc.m_imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        texDesc.m_sharingMode = vk::SharingMode::eExclusive;
        texDesc.m_imageLayout = vk::ImageLayout::eUndefined;
        return texDesc;
    }

    Texture::Texture(const LogicalDevice* device, const TextureDescriptor& desc,
        std::vector<std::uint32_t> queueFamilyIndices) :
        m_device{ device },
        m_imageDeviceMemory{ device, desc.m_memoryProperties },
        m_size{ 0 },
        m_mipLevels{ 0 },
        m_desc{ desc },
        m_queueFamilyIndices{ std::move(queueFamilyIndices) }
    {
        assert(m_desc.m_bytesPP > 0);
    }

    Texture::Texture(Texture&& rhs) noexcept :
        m_device{ rhs.m_device },
        m_vkImage{ std::move(rhs.m_vkImage) },
        m_vkImageView{ std::move(rhs.m_vkImageView) },
        m_imageDeviceMemory{ std::move(rhs.m_imageDeviceMemory) },
        m_size{ rhs.m_size },
        m_mipLevels{ rhs.m_mipLevels },
        m_desc{ rhs.m_desc },
        m_queueFamilyIndices{ std::move(rhs.m_queueFamilyIndices) },
        m_type{ rhs.m_type },
        m_viewType{ rhs.m_viewType }
    {
        rhs.m_size = glm::u32vec4(0);
        rhs.m_mipLevels = 0;
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        this->~Texture();
        m_device = rhs.m_device;
        m_vkImage = std::move(rhs.m_vkImage);
        m_vkImageView = std::move(rhs.m_vkImageView);
        m_imageDeviceMemory = std::move(rhs.m_imageDeviceMemory);
        m_size = rhs.m_size;
        m_mipLevels = rhs.m_mipLevels;
        m_desc = rhs.m_desc;
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        m_type = rhs.m_type;
        m_viewType = rhs.m_viewType;
        rhs.m_size = glm::u32vec4(0);
        rhs.m_mipLevels = 0;
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

        m_size = glm::u32vec4(size.x * m_desc.m_bytesPP, size.y, size.z, size.w);
        m_mipLevels = mipLevels;
        if (size.z == 1 && size.y == 1 && size.w == 1) { m_type = vk::ImageType::e1D; m_viewType = vk::ImageViewType::e1D; }
        else if (size.z == 1 && size.y == 1) { m_type = vk::ImageType::e1D; m_viewType = vk::ImageViewType::e1DArray; }
        else if (size.z == 1 && size.w == 1) { m_type = vk::ImageType::e2D; m_viewType = vk::ImageViewType::e2D; }
        else if (size.z == 1) { m_type = vk::ImageType::e2D; m_viewType = vk::ImageViewType::e2DArray; }
        // else if (size.z == 6 && size.w == 1) { m_type = vk::ImageType::e2D; m_viewType = vk::ImageViewType::eCubeMap; }
        // TODO: Add cube map support later. [3/17/2017 Sebastian Maisch]

        vk::ImageCreateInfo imgCreateInfo{ m_desc.m_createFlags, m_type, m_desc.m_format,
            vk::Extent3D(size.x, m_size.y, m_size.z),
            mipLevels, m_size.w, m_desc.m_samples, m_desc.m_imageTiling,
            m_desc.m_imageUsage, m_desc.m_sharingMode, 0, nullptr, m_desc.m_imageLayout };
        if (!m_queueFamilyIndices.empty()) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(m_queueFamilyIndices.size()));
            imgCreateInfo.setPQueueFamilyIndices(m_queueFamilyIndices.data());
        }

        m_vkImage = m_device->GetDevice().createImageUnique(imgCreateInfo);

        if (initMemory) {
            vk::MemoryRequirements memRequirements = m_device->GetDevice().getImageMemoryRequirements(*m_vkImage);
            m_imageDeviceMemory.InitializeMemory(memRequirements);
            m_imageDeviceMemory.BindToTexture(*this, 0);
            InitializeImageView();
        }
    }

    void Texture::InitializeImageView()
    {
        vk::ImageSubresourceRange imgSubresourceRange{ GetValidAspects(), 0, m_mipLevels, 0, m_size.w };
        vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), *m_vkImage, m_viewType, m_desc.m_format, vk::ComponentMapping(), imgSubresourceRange };
        m_vkImageView = m_device->GetDevice().createImageViewUnique(imgViewCreateInfo);
    }

    void Texture::TransitionLayout(vk::ImageLayout newLayout, vk::CommandBuffer cmdBuffer)
    {
        vk::ImageSubresourceRange subresourceRange{ GetValidAspects(), 0, m_mipLevels, 0, m_size.w };
        vk::ImageMemoryBarrier transitionBarrier{ vk::AccessFlags(), vk::AccessFlags(), m_desc.m_imageLayout,
            newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *m_vkImage, subresourceRange };

        transitionBarrier.srcAccessMask = GetAccessFlagsForLayout(m_desc.m_imageLayout);
        transitionBarrier.dstAccessMask = GetAccessFlagsForLayout(newLayout);

        cmdBuffer.pipelineBarrier(GetStageFlagsForLayout(m_desc.m_imageLayout), GetStageFlagsForLayout(newLayout),
            vk::DependencyFlags(), nullptr, nullptr, transitionBarrier);
        m_desc.m_imageLayout = newLayout;
    }

    vk::UniqueCommandBuffer Texture::TransitionLayout(vk::ImageLayout newLayout,
        std::pair<std::uint32_t, std::uint32_t> transitionQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence)
    {
        if (m_desc.m_imageLayout == newLayout) { return vk::UniqueCommandBuffer(); }

        auto transitionCmdBuffer = CommandBuffers::beginSingleTimeSubmit(m_device, transitionQueueIdx.first);
        TransitionLayout(newLayout, *transitionCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(m_device, *transitionCmdBuffer, transitionQueueIdx.first,
                                            transitionQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);
        return transitionCmdBuffer;
    }

    void Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4 & srcOffset, const Texture & dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4 & dstOffset, const glm::u32vec4 & size, vk::CommandBuffer cmdBuffer) const
    {
        assert(m_desc.m_imageUsage & vk::ImageUsageFlagBits::eTransferSrc);
        assert(dstImage.m_desc.m_imageUsage & vk::ImageUsageFlagBits::eTransferDst);
        assert(srcOffset.x + size.x <= m_size.x);
        assert(srcOffset.y + size.y <= m_size.y);
        assert(srcOffset.z + size.z <= m_size.z);
        assert(srcOffset.w + size.w <= m_size.w);
        assert(dstOffset.x + size.x <= dstImage.m_size.x);
        assert(dstOffset.y + size.y <= dstImage.m_size.y);
        assert(dstOffset.z + size.z <= dstImage.m_size.z);
        assert(dstOffset.w + size.w <= dstImage.m_size.w);
        assert(srcMipLevel < m_mipLevels);
        assert(dstMipLevel < dstImage.m_mipLevels);

        vk::ImageSubresourceLayers subresourceLayersSrc{ GetValidAspects(), srcMipLevel, srcOffset.w, size.w };
        vk::ImageSubresourceLayers subresourceLayersDst{ dstImage.GetValidAspects(), dstMipLevel, dstOffset.w, size.w };
        vk::ImageCopy copyRegion{ subresourceLayersSrc,
            vk::Offset3D{ static_cast<std::int32_t>(srcOffset.x), static_cast<std::int32_t>(srcOffset.y), static_cast<std::int32_t>(srcOffset.z) },
            subresourceLayersDst,
            vk::Offset3D{ static_cast<std::int32_t>(dstOffset.x), static_cast<std::int32_t>(dstOffset.y), static_cast<std::int32_t>(dstOffset.z) },
            vk::Extent3D{size.x / static_cast<std::uint32_t>(m_desc.m_bytesPP), size.y, size.z}};
        cmdBuffer.copyImage(*m_vkImage, m_desc.m_imageLayout, *dstImage.m_vkImage, dstImage.m_desc.m_imageLayout, copyRegion);
    }

    vk::UniqueCommandBuffer Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, const Texture& dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
        std::pair<std::uint32_t, std::uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        auto transferCmdBuffer = CommandBuffers::beginSingleTimeSubmit(m_device, copyQueueIdx.first);
        CopyImageAsync(srcMipLevel, srcOffset, dstImage, dstMipLevel, dstOffset, size, *transferCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(m_device, *transferCmdBuffer, copyQueueIdx.first, copyQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    vk::UniqueCommandBuffer Texture::CopyImageAsync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), m_size, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Texture::CopyImageSync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyImageAsync(dstImage, copyQueueIdx);
        m_device->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();
    }

    vk::ImageAspectFlags Texture::GetValidAspects() const
    {
        // TODO: support of the metadata aspect?
        if (m_desc.m_format == vk::Format::eD16Unorm || m_desc.m_format == vk::Format::eX8D24UnormPack32
            || m_desc.m_format == vk::Format::eD32Sfloat) {
            return vk::ImageAspectFlagBits::eDepth;
        }
        if (m_desc.m_format == vk::Format::eS8Uint) { return vk::ImageAspectFlagBits::eStencil; }
        if (m_desc.m_format == vk::Format::eD16UnormS8Uint || m_desc.m_format == vk::Format::eD24UnormS8Uint
            || m_desc.m_format == vk::Format::eD32SfloatS8Uint) {
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
