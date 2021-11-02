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
        return texDesc;
    }

    TextureDescriptor TextureDescriptor::StagingTextureDesc(const TextureDescriptor& orig)
    {
        TextureDescriptor texDesc = orig;
        texDesc.m_imageTiling = vk::ImageTiling::eLinear;
        texDesc.m_imageUsage = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.m_sharingMode = vk::SharingMode::eExclusive;
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
        return texDesc;
    }

    Texture::Texture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
        vk::ImageLayout initialLayout, std::vector<std::uint32_t> queueFamilyIndices)
        : VulkanObjectPrivateWrapper{device->GetHandle(), name, vk::UniqueImage{}}
        , MemoryBoundResource{device}
        , m_imageDeviceMemory{device, fmt::format("DeviceMemory:{}", name), desc.m_memoryProperties}
        , m_imageView{device->GetHandle(), fmt::format("View:{}", name), vk::UniqueImageView{}}
        , m_size{ 0 }
        , m_pixelSize{ 0 }
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
            vk::MemoryRequirements memRequirements = GetMemoryRequirements();
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

        SetObjectName(m_device->GetHandle(), m_image, GetName());

        if (initView) { InitializeImageView(); }
    }

    void Texture::InitializeImageView()
    {
        vk::ImageSubresourceRange imgSubresourceRange{ GetValidAspects(), 0, m_mipLevels, 0, m_size.w };

        vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), m_image, m_viewType, m_desc.m_format, vk::ComponentMapping(), imgSubresourceRange };
        m_imageView.SetHandle(m_device->GetHandle(), m_device->GetHandle().createImageViewUnique(imgViewCreateInfo));
    }

    void Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, Texture& dstImage,
        std::uint32_t dstMipLevel, const glm::u32vec4 & dstOffset, const glm::u32vec4 & size, CommandBuffer& cmdBuffer)
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

        PipelineBarrier barrier{m_device};
        AccessBarrier(vk::AccessFlagBits2KHR::eTransferRead, vk::PipelineStageFlagBits2KHR::eTransfer,
                      vk::ImageLayout::eTransferSrcOptimal, barrier);
        dstImage.AccessBarrier(vk::AccessFlagBits2KHR::eTransferWrite, vk::PipelineStageFlagBits2KHR::eTransfer,
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

    CommandBuffer Texture::CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, Texture& dstImage,
                                          std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset,
                                          const glm::u32vec4& size, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores,
                                          std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence)
    {
        auto transferCmdBuffer = CommandBuffer::beginSingleTimeSubmit(
            m_device, fmt::format("CopyImageAsyncCmdBuffer:{}-{}", GetName(), dstImage.GetName()), "CopyImageAsync",
            copyQueue.GetCommandPool());
        CopyImageAsync(srcMipLevel, srcOffset, dstImage, dstMipLevel, dstOffset, size, transferCmdBuffer);
        auto submitFence =
            CommandBuffer::endSingleTimeSubmit(copyQueue, transferCmdBuffer, waitSemaphores, signalSemaphores);
        if (fence.has_value()) { fence = submitFence; }

        return transferCmdBuffer;
    }

    CommandBuffer Texture::CopyImageAsync(Texture& dstImage, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores,
                                          std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence)
    {
        return CopyImageAsync(0, glm::u32vec4(0), dstImage, 0, glm::u32vec4(0), m_size, copyQueue, waitSemaphores,
                              signalSemaphores, fence);
    }

    void Texture::CopyImageSync(Texture& dstImage, const Queue& copyQueue)
    {
        auto cmdBuffer = CopyImageAsync(dstImage, copyQueue);
        copyQueue.WaitIdle();
    }

    void Texture::AccessBarrier(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                vk::ImageLayout imageLayout, PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, m_image, imageLayout, access, pipelineStages);
    }

    vk::Image Texture::GetImage(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                vk::ImageLayout imageLayout, PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, m_image, imageLayout, access, pipelineStages);
        return m_image;
    }

    const ImageView& Texture::GetImageView(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                           vk::ImageLayout imageLayout, PipelineBarrier& barrier)
    {

        barrier.AddSingleBarrier(this, m_image, imageLayout, access, pipelineStages);
        return m_imageView;
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

    void Texture::InitSize(const glm::u32vec4& size, std::uint32_t mipLevels)
    {
        assert(size.x > 0);
        assert(size.y > 0);
        assert(size.z > 0);
        assert(size.w > 0);
        assert(mipLevels > 0);

        // not sure if this is needed again at some point.
        assert(!m_image);
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

    vk::Image Texture::GetAccessNoBarrier() const { return m_image; }

     inline void Texture::BindMemory(vk::DeviceMemory deviceMemory, std::size_t offset)
     {
         m_device->GetHandle().bindImageMemory(GetHandle(), deviceMemory, offset);
     }

     vk::MemoryRequirements Texture::GetMemoryRequirements() const
     {
         return m_device->GetHandle().getImageMemoryRequirements(m_image);
     }

     vk::SubresourceLayout Texture::GetSubresourceLayout(const vk::ImageSubresource& subresource) const
     {
         return GetDevice().getImageSubresourceLayout(m_image, subresource);
     }

}
