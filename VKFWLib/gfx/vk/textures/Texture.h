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

namespace vku { namespace gfx {

    struct TextureDescriptor final
    {
        /** Holds the bytes per pixel of the format. */
        unsigned int bytesPP_;
        /** Holds the image create flags. */
        vk::ImageCreateFlags createFlags_;
        /** Holds the textures format. */
        vk::Format format_;
        /** Holds the number of samples. */
        vk::SampleCountFlagBits samples_ = vk::SampleCountFlagBits::e1;
        /** Holds the image tiling. */
        vk::ImageTiling imageTiling_;
        /** Holds the images usage flags. */
        vk::ImageUsageFlags imageUsage_;
        /** Holds the images sharing mode. */
        vk::SharingMode sharingMode_;
        /** Holds the images layout. */
        vk::ImageLayout imageLayout_;
        /** Holds the memory properties. */
        vk::MemoryPropertyFlags memoryProperties_;

        TextureDescriptor(const TextureDescriptor& desc, vk::MemoryPropertyFlags memProperties) :
            bytesPP_{desc.bytesPP_}, createFlags_{ desc.createFlags_ },
            format_{ desc.format_ }, samples_{ desc.samples_ },
            imageTiling_{ desc.imageTiling_ }, imageUsage_{ desc.imageUsage_ },
            sharingMode_{ desc.sharingMode_ }, imageLayout_{ desc.imageLayout_ },
            memoryProperties_{ desc.memoryProperties_ | memProperties } {}
        TextureDescriptor(const TextureDescriptor& desc, vk::ImageUsageFlags imageUsage) :
            bytesPP_{ desc.bytesPP_ }, createFlags_{ desc.createFlags_ },
            format_{ desc.format_ }, samples_{ desc.samples_ },
            imageTiling_{ desc.imageTiling_ },
            imageUsage_{ desc.imageUsage_ | imageUsage },
            sharingMode_{ desc.sharingMode_ }, imageLayout_{ desc.imageLayout_ },
            memoryProperties_{ desc.memoryProperties_ } {}
        TextureDescriptor(unsigned int bytesPP, vk::Format format, vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1) :
            bytesPP_{ bytesPP }, format_{ format }, samples_{ samples } {}

        static TextureDescriptor StagingTextureDesc(unsigned int bytesPP, vk::Format format, vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        static TextureDescriptor StagingTextureDesc(const TextureDescriptor orig);
        static TextureDescriptor SampleOnlyTextureDesc(unsigned int bytesPP, vk::Format format, vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        // static TextureDescriptor RenderTargetTextureDesc();
    };

    class Texture
    {
    public:
        Texture(const LogicalDevice* device, const TextureDescriptor& desc,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;
        ~Texture();

        void InitializeImage(const glm::u32vec4& size, uint32_t mipLevels, bool initMemory = true);
        void TransitionLayout(vk::ImageLayout newLayout) const;
        vk::CommandBuffer CopyImageAsync(uint32_t srcMipLevel, const glm::u32vec4& srcOffset,
            const Texture& dstImage, uint32_t dstMipLevel, const glm::u32vec4& dstOffset,
            const glm::u32vec4& size, std::pair<uint32_t, uint32_t> copyQueueIdx,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence()) const;
        vk::CommandBuffer CopyImageAsync(const Texture& dstImage, std::pair<uint32_t, uint32_t> copyQueueIdx,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence()) const;
        void CopyImageSync(const Texture& dstImage, std::pair<uint32_t, uint32_t> copyQueueIdx) const;

        const glm::u32vec4& GetSize() const { return size_; }
        uint32_t GetMipLevels() const { return mipLevels_; }
        vk::Image GetImage() const { return vkImage_; }
        const DeviceMemory& GetDeviceMemory() const { return imageDeviceMemory_; }

    protected:
        Texture CopyWithoutData() const { return Texture{ device_, desc_, queueFamilyIndices_ }; }
        vk::ImageAspectFlags GetValidAspects() const;
        const vk::Device& GetDevice() const { return device_->GetDevice(); }

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the Vulkan image object. */
        vk::Image vkImage_;
        /** Holds the Vulkan device memory for the image. */
        DeviceMemory imageDeviceMemory_;
        /** Holds the current size of the texture (x: bytes of line, y: #lines, z: #depth slices, w: #array slices). */
        glm::u32vec4 size_;
        /** Holds the number of MIP levels. */
        uint32_t mipLevels_;
        /** Holds the texture description. */
        TextureDescriptor desc_;
        /** Holds the queue family indices. */
        std::vector<uint32_t> queueFamilyIndices_;
    };
}}