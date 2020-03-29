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
        std::size_t bytesPP_;
        /** Holds the image create flags. */
        vk::ImageCreateFlags createFlags_ = vk::ImageCreateFlags();
        /** Holds the textures format. */
        vk::Format format_ = vk::Format();
        /** Holds the number of samples. */
        vk::SampleCountFlagBits samples_ = vk::SampleCountFlagBits::e1;
        /** Holds the image tiling. */
        vk::ImageTiling imageTiling_ = vk::ImageTiling();
        /** Holds the images usage flags. */
        vk::ImageUsageFlags imageUsage_ = vk::ImageUsageFlags();
        /** Holds the images sharing mode. */
        vk::SharingMode sharingMode_ = vk::SharingMode();
        /** Holds the images layout. */
        vk::ImageLayout imageLayout_ = vk::ImageLayout();
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_ = vk::MemoryPropertyFlags();

        TextureDescriptor(const TextureDescriptor& desc, const vk::MemoryPropertyFlags& memProperties) :
            bytesPP_{ desc.bytesPP_ }, createFlags_{ desc.createFlags_ },
            format_{ desc.format_ }, samples_{ desc.samples_ },
            imageTiling_{ desc.imageTiling_ }, imageUsage_{ desc.imageUsage_ },
            sharingMode_{ desc.sharingMode_ }, imageLayout_{ desc.imageLayout_ },
            memoryProperties_{ desc.memoryProperties_ | memProperties } {}
        TextureDescriptor(const TextureDescriptor& desc, const vk::ImageUsageFlags& imageUsage) :
            bytesPP_{ desc.bytesPP_ }, createFlags_{ desc.createFlags_ },
            format_{ desc.format_ }, samples_{ desc.samples_ },
            imageTiling_{ desc.imageTiling_ },
            imageUsage_{ desc.imageUsage_ | imageUsage },
            sharingMode_{ desc.sharingMode_ }, imageLayout_{ desc.imageLayout_ },
            memoryProperties_{ desc.memoryProperties_ } {}
        TextureDescriptor(std::size_t bytesPP, vk::Format format,
                          vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
            :
            bytesPP_{ bytesPP }, format_{ format }, samples_{ samples } {}

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
        Texture(const LogicalDevice* device, const TextureDescriptor& desc,
            std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;
        virtual ~Texture();

        void InitializeImage(const glm::u32vec4& size, std::uint32_t mipLevels, bool initMemory = true);
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

        [[nodiscard]] const glm::u32vec4& GetSize() const { return size_; }
        [[nodiscard]] std::uint32_t GetMipLevels() const { return mipLevels_; }
        [[nodiscard]] vk::Image GetImage() const { return *vkImage_; }
        [[nodiscard]] vk::ImageView GetImageView() const { return *vkImageView_; }
        [[nodiscard]] const DeviceMemory& GetDeviceMemory() const { return imageDeviceMemory_; }
        [[nodiscard]] const TextureDescriptor& GetDescriptor() const { return desc_; }

    protected:
        [[nodiscard]] Texture CopyWithoutData() const { return Texture{device_, desc_, queueFamilyIndices_}; }
        [[nodiscard]] vk::ImageAspectFlags GetValidAspects() const;
        [[nodiscard]] const vk::Device& GetDevice() const { return device_->GetDevice(); }
        static vk::AccessFlags GetAccessFlagsForLayout(vk::ImageLayout layout);
        static vk::PipelineStageFlags GetStageFlagsForLayout(vk::ImageLayout layout);

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan image object. */
        vk::UniqueImage vkImage_;
        /** Holds the Vulkan image view. */
        vk::UniqueImageView vkImageView_;
        /** Holds the Vulkan device memory for the image. */
        DeviceMemory imageDeviceMemory_;
        /** Holds the current size of the texture (x: bytes of line, y: #lines, z: #depth slices, w: #array slices). */
        glm::u32vec4 size_;
        /** Holds the number of MIP levels. */
        std::uint32_t mipLevels_;
        /** Holds the texture description. */
        TextureDescriptor desc_;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> queueFamilyIndices_;
        /** Holds the image type. */
        vk::ImageType type_ = vk::ImageType::e3D;
        /** Holds the image view type. */
        vk::ImageViewType viewType_ = vk::ImageViewType::e3D;
    };
}
