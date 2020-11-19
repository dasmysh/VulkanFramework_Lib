/**
 * @file   Texture.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a texture object for Vulkan.
 */

#pragma once

#include "main.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/DeviceMemory.h"

#include <glm/gtc/type_precision.hpp>

namespace vkfw_core::gfx {

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
        /** Holds the images layout. */
        vk::ImageLayout m_imageLayout = vk::ImageLayout();
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags m_memoryProperties = vk::MemoryPropertyFlags();

        TextureDescriptor(const TextureDescriptor& desc, const vk::MemoryPropertyFlags& memProperties)
            : m_bytesPP{desc.m_bytesPP},
              m_createFlags{desc.m_createFlags},
              m_format{desc.m_format},
              m_samples{desc.m_samples},
              m_imageTiling{desc.m_imageTiling},
              m_imageUsage{desc.m_imageUsage},
              m_sharingMode{desc.m_sharingMode},
              m_imageLayout{desc.m_imageLayout},
              m_memoryProperties{desc.m_memoryProperties | memProperties}
        {
        }
        TextureDescriptor(const TextureDescriptor& desc, const vk::ImageUsageFlags& imageUsage)
            : m_bytesPP{desc.m_bytesPP},
              m_createFlags{desc.m_createFlags},
              m_format{desc.m_format},
              m_samples{desc.m_samples},
              m_imageTiling{desc.m_imageTiling},
              m_imageUsage{desc.m_imageUsage | imageUsage},
              m_sharingMode{desc.m_sharingMode},
              m_imageLayout{desc.m_imageLayout},
              m_memoryProperties{desc.m_memoryProperties}
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
        // static TextureDescriptor RenderTargetTextureDesc();
    };

    class Texture
    {
    public:
        /** Create image. */
        Texture(const LogicalDevice* device, const TextureDescriptor& desc,
            std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;
        virtual ~Texture();

        void InitializeImage(const glm::u32vec4& size, std::uint32_t mipLevels, bool initMemory = true);
        void InitializeExternalImage(vk::Image externalImage, const glm::u32vec4& size, std::uint32_t mipLevels, bool initView = true);
        void InitializeImageView();
        void TransitionLayout(vk::ImageLayout newLayout, vk::CommandBuffer cmdBuffer);
        vk::UniqueCommandBuffer TransitionLayout(vk::ImageLayout newLayout,
            std::pair<std::uint32_t, std::uint32_t> transitionQueueIdx,
            const std::vector<vk::Semaphore>& waitSemaphores,
            const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence);
        void CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset,
            const Texture& dstImage, std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
                            vk::CommandBuffer cmdBuffer) const;
        [[nodiscard]] vk::UniqueCommandBuffer
        CopyImageAsync(std::uint32_t srcMipLevel, const glm::u32vec4& srcOffset, const Texture& dstImage,
                       std::uint32_t dstMipLevel, const glm::u32vec4& dstOffset, const glm::u32vec4& size,
                       std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
                       const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
                       const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
                       vk::Fence fence = vk::Fence()) const;
        [[nodiscard]] vk::UniqueCommandBuffer
        CopyImageAsync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
                       const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
                       const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
                       vk::Fence fence = vk::Fence()) const;
        void CopyImageSync(const Texture& dstImage, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const;

        [[nodiscard]] const glm::u32vec4& GetSize() const { return m_size; }
        [[nodiscard]] const glm::u32vec4& GetPixelSize() const { return m_pixelSize; }
        [[nodiscard]] std::uint32_t GetMipLevels() const { return m_mipLevels; }
        [[nodiscard]] vk::Image GetImage() const { return m_vkImage; }
        [[nodiscard]] vk::ImageView GetImageView() const { return *m_vkImageView; }
        [[nodiscard]] const DeviceMemory& GetDeviceMemory() const { return m_imageDeviceMemory; }
        [[nodiscard]] const TextureDescriptor& GetDescriptor() const { return m_desc; }

    protected:
        [[nodiscard]] Texture CopyWithoutData() const { return Texture{m_device, m_desc, m_queueFamilyIndices}; }
        [[nodiscard]] vk::ImageAspectFlags GetValidAspects() const;
        [[nodiscard]] const vk::Device& GetDevice() const { return m_device->GetDevice(); }
        static vk::AccessFlags GetAccessFlagsForLayout(vk::ImageLayout layout);
        static vk::PipelineStageFlags GetStageFlagsForLayout(vk::ImageLayout layout);

    private:
        void InitSize(const glm::u32vec4& size, std::uint32_t mipLevels);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the Vulkan internal image object. */
        vk::UniqueImage m_vkInternalImage;
        /** Holds the Vulkan image object for external and internal images. */
        vk::Image m_vkImage = vk::Image{};
        /** Holds the Vulkan image view. */
        vk::UniqueImageView m_vkImageView;
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
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> m_queueFamilyIndices;
        /** Holds the image type. */
        vk::ImageType m_type = vk::ImageType::e3D;
        /** Holds the image view type. */
        vk::ImageViewType m_viewType = vk::ImageViewType::e3D;
    };
}
