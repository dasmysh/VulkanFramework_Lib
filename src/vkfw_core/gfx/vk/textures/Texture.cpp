/**
 * @file   Texture.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a texture object for Vulkan.
 */

#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/vk/wrappers/CommandBuffer.h"

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

    Texture::Texture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
        vk::ImageLayout initialLayout, std::vector<std::uint32_t> queueFamilyIndices)
        : VulkanObjectPrivateWrapper{device->GetHandle(), name, vk::UniqueImage{}}
        , MemoryBoundResource{device}
        , m_imageDeviceMemory{device, fmt::format("DeviceMemory:{}", name), desc.m_memoryProperties}
        , m_imageView{device->GetHandle(), fmt::format("View:{}", name), vk::UniqueImageView{}}
        , m_size{ 0 }
        , m_mipLevels{ 0 }
        , m_desc{ desc }
        , m_imageLayout{initialLayout}
        , m_queueFamilyIndices{ std::move(queueFamilyIndices) }
    {
        assert(m_desc.m_bytesPP > 0);
    }

    Texture::Texture(Texture&& rhs) noexcept
        : VulkanObjectPrivateWrapper{std::move(rhs)}
        , MemoryBoundResource{std::move(rhs)}
        , m_image{std::move(rhs.m_image)}
        , m_imageView{std::move(rhs.m_imageView)}
        , m_imageDeviceMemory{std::move(rhs.m_imageDeviceMemory)}
        , m_size{rhs.m_size}
        , m_pixelSize{rhs.m_pixelSize}
        , m_mipLevels{rhs.m_mipLevels}
        , m_desc{rhs.m_desc}
        , m_imageLayout{rhs.m_imageLayout}
        , m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
        , m_type{rhs.m_type}, m_viewType{rhs.m_viewType}
    {
        rhs.m_size = glm::u32vec4(0);
        rhs.m_pixelSize = glm::u32vec4(0);
        rhs.m_mipLevels = 0;
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        this->~Texture();
        VulkanObjectPrivateWrapper::operator=(std::move(rhs));
        MemoryBoundResource::operator=(std::move(rhs));
        m_device = rhs.m_device;
        m_image = std::move(rhs.m_image);
        m_imageView = std::move(rhs.m_imageView);
        m_imageDeviceMemory = std::move(rhs.m_imageDeviceMemory);
        m_size = rhs.m_size;
        m_pixelSize = rhs.m_pixelSize;
        m_mipLevels = rhs.m_mipLevels;
        m_desc = rhs.m_desc;
        m_imageLayout = rhs.m_imageLayout;
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        m_type = rhs.m_type;
        m_viewType = rhs.m_viewType;
        rhs.m_size = glm::u32vec4(0);
        rhs.m_pixelSize = glm::u32vec4(0);
        rhs.m_mipLevels = 0;
        return *this;
    }

    Texture::~Texture() = default;

    void Texture::InitializeImage(const glm::u32vec4& size, std::uint32_t mipLevels, bool initMemory)
    {
        InitSize(size, mipLevels);

        vk::ImageCreateInfo imgCreateInfo{ m_desc.m_createFlags, m_type, m_desc.m_format,
            vk::Extent3D(size.x, m_size.y, m_size.z),
            mipLevels, m_size.w, m_desc.m_samples, m_desc.m_imageTiling,
            m_desc.m_imageUsage, m_desc.m_sharingMode, 0, nullptr, GetImageLayout() };
        if (!m_queueFamilyIndices.empty()) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(m_queueFamilyIndices.size()));
            imgCreateInfo.setPQueueFamilyIndices(m_queueFamilyIndices.data());
        }

        SetHandle(m_device->GetHandle(), m_device->GetHandle().createImageUnique(imgCreateInfo));
        m_image = GetHandle();

        if (initMemory) {
            vk::MemoryRequirements memRequirements =
                m_device->GetHandle().getImageMemoryRequirements(GetHandle());
            m_imageDeviceMemory.InitializeMemory(memRequirements);
            BindMemory(m_imageDeviceMemory.GetHandle(), 0);
            InitializeImageView();
        }
    }

    void Texture::InitializeExternalImage(vk::Image externalImage, const glm::u32vec4& size, std::uint32_t mipLevels,
                                          bool initView)
    {
        InitSize(size, mipLevels);
        m_image = externalImage;

        if (initView) { InitializeImageView(); }
    }

    void Texture::InitializeImageView()
    {
        vk::ImageSubresourceRange imgSubresourceRange{ GetValidAspects(), 0, m_mipLevels, 0, m_size.w };

        vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), m_image, m_viewType, m_desc.m_format, vk::ComponentMapping(), imgSubresourceRange };
        m_imageView.SetHandle(m_device->GetHandle(), m_device->GetHandle().createImageViewUnique(imgViewCreateInfo));
    }

    void Texture::TransitionLayout(vk::ImageLayout newLayout, const CommandBuffer& cmdBuffer)
    {
        vk::ImageSubresourceRange subresourceRange{ GetValidAspects(), 0, m_mipLevels, 0, m_size.w };
        vk::ImageMemoryBarrier transitionBarrier{ vk::AccessFlags(), vk::AccessFlags(), m_desc.m_imageLayout,
            newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_image, subresourceRange };

        transitionBarrier.srcAccessMask = GetAccessFlagsForLayout(m_desc.m_imageLayout);
        transitionBarrier.dstAccessMask = GetAccessFlagsForLayout(newLayout);

        cmdBuffer.GetHandle().pipelineBarrier(GetStageFlagsForLayout(m_desc.m_imageLayout), GetStageFlagsForLayout(newLayout),
            vk::DependencyFlags(), nullptr, nullptr, transitionBarrier);
        m_desc.m_imageLayout = newLayout;
    }

    CommandBuffer Texture::TransitionLayout(vk::ImageLayout newLayout, const Queue& transitionQueue,
                                            std::span<vk::Semaphore> waitSemaphores,
                                            std::span<vk::Semaphore> signalSemaphores, const Fence& fence)
    {
        if (m_desc.m_imageLayout == newLayout) { return CommandBuffer{}; }

        auto transitionCmdBuffer = CommandBuffer::beginSingleTimeSubmit(
            m_device, fmt::format("TransitionLayoutCmdBuffer:{}", GetName()), "TransitionLayout",
            transitionQueue.GetCommandPool());
        TransitionLayout(newLayout, transitionCmdBuffer);
        CommandBuffer::endSingleTimeSubmit(transitionQueue, transitionCmdBuffer, waitSemaphores, signalSemaphores,
                                           fence);
        return transitionCmdBuffer;
    }

    void Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, Texture& dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4 & dstOffset, const glm::u32vec4 & size, const CommandBuffer& cmdBuffer)
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

        auto srcAccessor = GetAccess();
        auto dstAccessor = dstImage.GetAccess();

        PipelineBarrier barrier{m_device, vk::PipelineStageFlagBits::eTransfer};
        srcAccessor.Get(vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eTransfer,
                        vk::ImageLayout::eTransferSrcOptimal, barrier);
        dstAccessor.Get(vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer,
                        vk::ImageLayout::eTransferDstOptimal, barrier);
        barrier.Record(cmdBuffer);

        vk::ImageSubresourceLayers subresourceLayersSrc{ GetValidAspects(), srcMipLevel, srcOffset.w, size.w };
        vk::ImageSubresourceLayers subresourceLayersDst{ dstImage.GetValidAspects(), dstMipLevel, dstOffset.w, size.w };
        vk::ImageCopy copyRegion{ subresourceLayersSrc,
            vk::Offset3D{ static_cast<std::int32_t>(srcOffset.x), static_cast<std::int32_t>(srcOffset.y), static_cast<std::int32_t>(srcOffset.z) },
            subresourceLayersDst,
            vk::Offset3D{ static_cast<std::int32_t>(dstOffset.x), static_cast<std::int32_t>(dstOffset.y), static_cast<std::int32_t>(dstOffset.z) },
            vk::Extent3D{size.x / static_cast<std::uint32_t>(m_desc.m_bytesPP), size.y, size.z}};
        cmdBuffer.GetHandle().copyImage(m_image, GetImageLayout(), dstImage.m_image, dstImage.GetImageLayout(),
                                        copyRegion);
    }

    CommandBuffer Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset,
                                          Texture& dstImage, std::uint32_t dstMipLevel,
                                          const glm::u32vec4& dstOffset, const glm::u32vec4& size,
                                          const Queue& copyQueue, std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores, const Fence& fence)
    {
        auto transferCmdBuffer = CommandBuffer::beginSingleTimeSubmit(
            m_device, fmt::format("CopyImageAsyncCmdBuffer:{}-{}", GetName(), dstImage.GetName()),
            "CopyImageAsync", copyQueue.GetCommandPool());
        CopyImageAsync(srcMipLevel, srcOffset, dstImage, dstMipLevel, dstOffset, size, transferCmdBuffer);
        CommandBuffer::endSingleTimeSubmit(copyQueue, transferCmdBuffer, waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    CommandBuffer Texture::CopyImageAsync(Texture& dstImage, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores, const Fence& fence)
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), m_size, copyQueue, waitSemaphores,
                              signalSemaphores, fence);
    }

    void Texture::CopyImageSync(Texture& dstImage, const Queue& copyQueue)
    {
        auto cmdBuffer = CopyImageAsync(dstImage, copyQueue);
        copyQueue.WaitIdle();
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

    ImageAccessor Texture::GetAccess() { return ImageAccessor{m_device, GetHandle(), this}; }

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

    void Texture::InitSize(const glm::u32vec4& size, std::uint32_t mipLevels)
    {
        assert(size.x > 0);
        assert(size.y > 0);
        assert(size.z > 0);
        assert(size.w > 0);
        assert(mipLevels > 0);

        // not sure if this is needed again at some point.
        assert(!GetHandle());
        // this->~Texture();

        m_pixelSize = size;
        m_size = glm::u32vec4(size.x * m_desc.m_bytesPP, size.y, size.z, size.w);
        m_mipLevels = mipLevels;
        if (size.z == 1 && size.y == 1 && size.w == 1) {
            m_type = vk::ImageType::e1D;
            m_viewType = vk::ImageViewType::e1D;
        } else if (size.z == 1 && size.y == 1) {
            m_type = vk::ImageType::e1D;
            m_viewType = vk::ImageViewType::e1DArray;
        } else if (size.z == 1 && size.w == 1) {
            m_type = vk::ImageType::e2D;
            m_viewType = vk::ImageViewType::e2D;
        } else if (size.z == 1) {
            m_type = vk::ImageType::e2D;
            m_viewType = vk::ImageViewType::e2DArray;
        }
        // else if (size.z == 6 && size.w == 1) { m_type = vk::ImageType::e2D; m_viewType = vk::ImageViewType::eCubeMap; }
        // TODO: Add cube map support later. [3/17/2017 Sebastian Maisch]
    }

    void Texture::AddDescriptorLayoutBinding(DescriptorSetLayout& layout, vk::DescriptorType type,
                                             vk::ShaderStageFlags shaderFlags, std::uint32_t binding /*= 0*/)
    {
        assert(type == vk::DescriptorType::eCombinedImageSampler || type == vk::DescriptorType::eSampledImage
               || type == vk::DescriptorType::eStorageImage || type == vk::DescriptorType::eInputAttachment);
        layout.AddBinding(binding, type, 1, shaderFlags);
    }

    void Texture::FillDescriptorImageInfo(vk::DescriptorImageInfo& descInfo, const Sampler& sampler) const
    {
        descInfo.sampler = sampler.GetHandle();
        descInfo.imageView = m_imageView.GetHandle();
        descInfo.imageLayout = GetImageLayout();
    }


    ImageAccessor::ImageAccessor(const LogicalDevice* device, vk::Image image, Texture* texture)
        : m_device{device}, m_image{image}, m_texture{texture}
    {
    }

    ImageAccessor::ImageAccessor(ImageAccessor&& rhs) noexcept
        : m_device{rhs.m_device}, m_image{rhs.m_image}, m_texture{rhs.m_texture}
    {
        rhs.m_device = nullptr;
        rhs.m_image = nullptr;
        rhs.m_texture = nullptr;
    }

    ImageAccessor& ImageAccessor::operator=(ImageAccessor&& rhs) noexcept
    {
        m_device = rhs.m_device;
        m_image = rhs.m_image;
        m_texture = rhs.m_texture;
        rhs.m_device = nullptr;
        rhs.m_image = nullptr;
        rhs.m_texture = nullptr;
        return *this;
    }

    inline vk::Image ImageAccessor::Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages,
                                        vk::ImageLayout imageLayout, SingleResourcePipelineBarrier& barrier)
    {
        barrier = SingleResourcePipelineBarrier{m_device, m_texture, m_image, imageLayout, access, pipelineStages};
        return m_image;
    }

    inline vk::Image ImageAccessor::Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages,
                                        vk::ImageLayout imageLayout, PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(m_texture, m_image, imageLayout, access, pipelineStages);
        return m_image;
    }

}
