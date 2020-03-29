/**
 * @file   Framebuffer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a framebuffer object for Vulkan.
 */

#pragma once

#include "main.h"
#include "gfx/vk/textures/DeviceTexture.h"
#include "memory/DeviceMemoryGroup.h"

namespace vkfw_core::gfx {

    class LogicalDevice;

    struct FramebufferDescriptor final
    {
        /** Holds the texture descriptors. */
        std::vector<TextureDescriptor> tex_;
        /** Holds the textures image type. */
        vk::ImageViewType type_ = vk::ImageViewType::e2D;
    };

    class Framebuffer final
    {
    public:
        Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size, std::vector<vk::Image> images,
            const vk::RenderPass& renderPass, const FramebufferDescriptor& desc,
            std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{},
            vk::CommandBuffer cmdBuffer = vk::CommandBuffer());
        Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size, const vk::RenderPass& renderPass,
            const FramebufferDescriptor& desc, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{},
            vk::CommandBuffer cmdBuffer = vk::CommandBuffer());
        Framebuffer(const Framebuffer&);
        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(const Framebuffer&);
        Framebuffer& operator=(Framebuffer&&) noexcept;
        ~Framebuffer();

        [[nodiscard]] glm::uvec2 GetSize() const { return size_; }
        [[nodiscard]] unsigned int GetWidth() const { return size_.x; }
        [[nodiscard]] unsigned int GetHeight() const { return size_.y; }
        [[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return *vkFramebuffer_; }

    private:
        void CreateImages(vk::CommandBuffer cmdBuffer);
        void CreateFB();

        /** Holds the logical device. */
        const LogicalDevice* device_;
        /** Holds the framebuffer size. */
        glm::uvec2 size_;
        /** Holds the render pass. */
        vk::RenderPass renderPass_;
        /** Holds the framebuffer descriptor. */
        FramebufferDescriptor desc_;
        /** Holds the device memory group for the owned images. */
        DeviceMemoryGroup memoryGroup_;
        /** Holds the externally owned images in this framebuffer. */
        std::vector<vk::Image> extImages_;
        /** Holds the image view for the external attachments. */
        std::vector<vk::UniqueImageView> vkExternalAttachmentsImageView_;
        /** Holds the Vulkan framebuffer object. */
        vk::UniqueFramebuffer vkFramebuffer_;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> queueFamilyIndices_;
    };
}
