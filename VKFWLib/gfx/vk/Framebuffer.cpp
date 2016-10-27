/**
 * @file   Framebuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Implementation of a framebuffer object for Vulkan.
 */

#include "Framebuffer.h"
#include "LogicalDevice.h"

namespace vku { namespace gfx {

    Framebuffer::Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const std::vector<vk::Image>& images, const FramebufferDescriptor& desc) :
        logicalDevice_{ logicalDevice },
        vkAttachmentsImageView_{ desc.tex_.size() }
    {
        assert(desc.type_ == vk::ImageViewType::e2D || desc.type_ == vk::ImageViewType::eCube);
        assert(images.size() == desc.tex_.size());
        uint32_t layerCount = 1;
        if (desc.type_ == vk::ImageViewType::eCube) layerCount = 6;

        // TODO: handle depth buffers... [10/27/2016 Sebastian Maisch]
        for (auto i = 0U; i < desc.tex_.size(); ++i) {
            vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount };
            vk::ComponentMapping componentMapping{};
            vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), images[i], desc.type_, desc.tex_[i].format_, componentMapping, subresourceRange };
            vkAttachmentsImageView_[i] = logicalDevice_->GetDevice().createImageView(imgViewCreateInfo);
        }

        vk::FramebufferCreateInfo fbCreateInfo{ vk::FramebufferCreateFlags(), vkSwapchainRenderPass_,
            static_cast<uint32_t>(desc.tex_.size()), vkAttachmentsImageView_, size.x, size.y, layerCount };
        vkFramebuffer_ = logicalDevice_->GetDevice().createFramebuffer(fbCreateInfo);
    }

    Framebuffer::Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const FramebufferDescriptor& desc) :
        logicalDevice_{ logicalDevice },
        vkAttachmentsImageView_{ desc.tex_.size() }
    {
        // TODO: Create images... [10/27/2016 Sebastian Maisch]
    }
}}
