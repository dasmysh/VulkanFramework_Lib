/**
 * @file   Framebuffer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a framebuffer object for Vulkan.
 */

#pragma once

#include "main.h"
#include "Texture.h"

namespace vku {
    namespace gfx {

        class LogicalDevice;

        struct FramebufferDescriptor
        {
            /** Holds the texture descriptors. */
            std::vector<TextureDescriptor> tex_;
            /** Holds the textures image type. */
            vk::ImageViewType type_ = vk::ImageViewType::e2D;
        };

        class Framebuffer
        {
        public:
            Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const std::vector<vk::Image>& images, const FramebufferDescriptor& desc);
            Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const FramebufferDescriptor& desc);
            Framebuffer();
            Framebuffer(const Framebuffer&);
            Framebuffer(Framebuffer&&) noexcept;
            Framebuffer& operator=(const Framebuffer&);
            Framebuffer& operator=(Framebuffer&&) noexcept;

        private:
            void CreateRenderPass(const FramebufferDescriptor& desc);

            /** Holds the logical device. */
            LogicalDevice* logicalDevice_;
            /** Holds the image view for the attachments. */
            std::vector<vk::ImageView> vkAttachmentsImageView_;
            /** Holds the Vulkan framebuffer object. */
            vk::Framebuffer vkFramebuffer_;
        };
    }
}
