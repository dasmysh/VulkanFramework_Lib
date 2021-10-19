/**
 * @file   Texture.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a texture object for Vulkan.
 */

#pragma once

#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/ImageResources.h"
#include "gfx/vk/wrappers/MemoryBoundResource.h"
#include "gfx/vk/wrappers/Sampler.h"
#include "main.h"

#include <glm/gtc/type_precision.hpp>

namespace vkfw_core::gfx {

    class DescriptorSetLayout;
    class Queue;
    class Texture;

    struct TextureDescriptor final
    {
        /** Holds the bytes per pixel of the format. */
        std::size_t m_bytesPP;
        /** Holds the image create flags. */
        vk::ImageCreateFlags m_createFlags = vk::ImageCreateFlags();
        /** Holds the textures format. */
        vk::Format m_format = vk::Format();
        /** Holds the number of samples. */
        vk::SampleCountFlagBits m_samples = vk::SampleCountFlagBits::e1;
        /** Holds the image tiling. */
        vk::ImageTiling m_imageTiling = vk::ImageTiling();
        /** Holds the images usage flags. */
        vk::ImageUsageFlags m_imageUsage = vk::ImageUsageFlags();
        /** Holds the images sharing mode. */
        vk::SharingMode m_sharingMode = vk::SharingMode();
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags m_memoryProperties = vk::MemoryPropertyFlags();

        TextureDescriptor(const TextureDescriptor& desc, const vk::MemoryPropertyFlags& memProperties)
            : m_bytesPP{desc.m_bytesPP}
            , m_createFlags{desc.m_createFlags}
            , m_format{desc.m_format}
            , m_samples{desc.m_samples}
            , m_imageTiling{desc.m_imageTiling}
            , m_imageUsage{desc.m_imageUsage}
            , m_sharingMode{desc.m_sharingMode}
            , m_memoryProperties{desc.m_memoryProperties | memProperties}
        {
        }
        TextureDescriptor(const TextureDescriptor& desc, const vk::ImageUsageFlags& imageUsage)
            : m_bytesPP{desc.m_bytesPP}
            , m_createFlags{desc.m_createFlags}
            , m_format{desc.m_format}
            , m_samples{desc.m_samples}
            , m_imageTiling{desc.m_imageTiling}
            , m_imageUsage{desc.m_imageUsage | imageUsage}
            , m_sharingMode{desc.m_sharingMode}
            , m_memoryProperties{desc.m_memoryProperties}
        {
        }
        TextureDescriptor(std::size_t bytesPP, vk::Format format,
                          vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
            : m_bytesPP{bytesPP}, m_format{format}, m_samples{samples}
        {
        }

        static TextureDescriptor StagingTextureDesc(std::size_t bytesPP, vk::Format format,
                                                    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        static TextureDescriptor StagingTextureDesc(const TextureDescriptor& orig);
        static TextureDescriptor SampleOnlyTextureDesc(std::size_t bytesPP, vk::Format format,
                                                       vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        static TextureDescriptor DepthBufferTextureDesc(std::size_t bytesPP, vk::Format format,
                                                        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
    };

    class ImageAccessor
    {
    public:
        ImageAccessor(const LogicalDevice* device, vk::Image image, Texture* texture);
        ImageAccessor(ImageAccessor&& rhs) noexcept;
        ImageAccessor& operator=(ImageAccessor&&) noexcept;
        ImageAccessor(const ImageAccessor&) = delete;
        ImageAccessor& operator=(const ImageAccessor&) = delete;

        void SetAccess(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages, vk::ImageLayout imageLayout,
                       PipelineBarrier& barrier);
        [[nodiscard]] vk::Image Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages,
                                    vk::ImageLayout imageLayout, SingleResourcePipelineBarrier& barrier);
        [[nodiscard]] vk::Image Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages,
                                    vk::ImageLayout imageLayout, PipelineBarrier& barrier);

    private:
        const LogicalDevice* m_device;
        vk::Image m_image;
        Texture* m_texture;
    };

    class Texture : public VulkanObjectPrivateWrapper<vk::UniqueImage>, public MemoryBoundResource
    {
    public:
        /** Create image. */
        Texture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
                vk::ImageLayout initialLayout,
                std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;
        virtual ~Texture();

        static void AddDescriptorLayoutBinding(DescriptorSetLayout& layout, vk::DescriptorType type,
                                               vk::ShaderStageFlags shaderFlags, std::uint32_t binding = 0);
        void FillDescriptorImageInfo(vk::DescriptorImageInfo& descInfo, const Sampler& sampler, vk::ImageLayout imageLayout) const;

        void InitializeImage(const glm::u32vec4& size, std::uint32_t mipLevels, bool initMemory = true);
        void InitializeExternalImage(vk::Image externalImage, const glm::u32vec4& size, std::uint32_t mipLevels,
                                     bool initView = true);
        void InitializeImageView();

        void CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, Texture& dstImage,
                            std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
                            const CommandBuffer& cmdBuffer);
        [[nodiscard]] CommandBuffer
        CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, Texture& dstImage,
                       std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
                       const Queue& copyQueue, std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                       std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{},
                       const Fence& fence = Fence{});
        [[nodiscard]] CommandBuffer
        CopyImageAsync(Texture& dstImage, const Queue& transitionQueue,
                       std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                       std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{},
                       const Fence& fence = Fence{});
        void CopyImageSync(Texture& dstImage, const Queue& copyQueue);

        [[nodiscard]] const glm::u32vec4& GetSize() const { return m_size; }
        [[nodiscard]] const glm::u32vec4& GetPixelSize() const { return m_pixelSize; }
        [[nodiscard]] std::uint32_t GetMipLevels() const { return m_mipLevels; }
        // [[nodiscard]] vk::Image GetImage() const { return m_image; }
        [[nodiscard]] const ImageView& GetImageView() const { return m_imageView; }
        [[nodiscard]] const DeviceMemory& GetDeviceMemory() const { return m_imageDeviceMemory; }
        [[nodiscard]] const TextureDescriptor& GetDescriptor() const { return m_desc; }
        [[nodiscard]] vk::ImageAspectFlags GetValidAspects() const;
        [[nodiscard]] ImageAccessor GetAccess();
        [[nodiscard]] vk::Image GetAccessNoBarrier() const;
        [[nodiscard]] vk::ImageLayout GetImageLayout() const { return m_imageLayout; }
        [[nodiscard]] vk::MemoryRequirements GetMemoryRequirements() const;
        [[nodiscard]] vk::SubresourceLayout GetSubresourceLayout(const vk::ImageSubresource& subresource) const;
        void SetImageLayout(vk::ImageLayout layout) { m_imageLayout = layout; }
        void BindMemory(vk::DeviceMemory deviceMemory, std::size_t offset);

    protected:
        [[nodiscard]] Texture CopyWithoutData(std::string_view name) const
        {
            return Texture{m_device, name, m_desc, GetImageLayout(), m_queueFamilyIndices};
        }
        [[nodiscard]] vk::Device GetDevice() const { return m_device->GetHandle(); }

    private:
        void InitSize(const glm::u32vec4& size, std::uint32_t mipLevels);

        /** Holds the Vulkan image object for external and internal images. */
        vk::Image m_image = nullptr;
        /** Holds the Vulkan image view. */
        ImageView m_imageView;
        /** Holds the Vulkan device memory for the image. */
        DeviceMemory m_imageDeviceMemory;
        /** Holds the current size of the texture (x: bytes of line, y: #lines, z: #depth slices, w: #array slices). */
        glm::u32vec4 m_size;
        /** Holds the current size of the texture in pixels. */
        glm::u32vec4 m_pixelSize;
        /** Holds the number of MIP levels. */
        std::uint32_t m_mipLevels;
        /** Holds the texture description. */
        TextureDescriptor m_desc;
        /** Holds the images layout. */
        vk::ImageLayout m_imageLayout = vk::ImageLayout();
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> m_queueFamilyIndices;
        /** Holds the image type. */
        vk::ImageType m_type = vk::ImageType::e3D;
        /** Holds the image view type. */
        vk::ImageViewType m_viewType = vk::ImageViewType::e3D;
    };
}
