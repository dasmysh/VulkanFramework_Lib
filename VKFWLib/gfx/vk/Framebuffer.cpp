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
            // TODO: samples! [10/27/2016 Sebastian Maisch]
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

    void Framebuffer::CreateRenderPass(const FramebufferDescriptor& desc)
    {
        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

        std::vector<vk::AttachmentDescription> attachments;
        std::vector<vk::AttachmentReference> colorAttachmentReferences;
        attachments.reserve(desc.tex_.size());
        colorAttachmentReferences.resize(attachments.size());
        // Color attachment
        for (size_t i = 0; i < attachments.size(); ++i) {
            attachments.emplace_back(vk::AttachmentDescriptionFlags(), desc.tex_[i].format_, desc.tex_[i].samples_, vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, ImageLayout initialLayout_ = ImageLayout::eUndefined, ImageLayout finalLayout_ = ImageLayout::eUndefined)
            attachments[i].format = desc.tex_[i].format_;
            attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
            attachments[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
            attachments[i].initialLayout = vk::ImageLayout::eUndefined;
            attachments[i].finalLayout = colorFinalLayout;

            vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
            attachmentReference.attachment = i;
            attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

            subpass.colorAttachmentCount = colorAttachmentReferences.size();
            subpass.pColorAttachments = colorAttachmentReferences.data();
        }

        // Do we have a depth format?
        vk::AttachmentReference depthAttachmentReference;
        if (depthFormat != vk::Format::eUndefined) {
            vk::AttachmentDescription depthAttachment;
            depthAttachment.format = depthFormat;
            depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            // We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
            depthAttachment.storeOp = depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
            depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
            depthAttachment.finalLayout = depthFinalLayout;
            attachments.push_back(depthAttachment);
            depthAttachmentReference.attachment = attachments.size() - 1;
            depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            subpass.pDepthStencilAttachment = &depthAttachmentReference;
        }

        std::vector<vk::SubpassDependency> subpassDependencies;
        {
            if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
                // Implicit transition 
                vk::SubpassDependency dependency;
                dependency.srcSubpass = 0;
                dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

                dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
                dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
                subpassDependencies.push_back(dependency);
            }

            if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
                // Implicit transition 
                vk::SubpassDependency dependency;
                dependency.srcSubpass = 0;
                dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

                dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
                dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
                subpassDependencies.push_back(dependency);
            }
        }

        if (renderPass) {
            context.device.destroyRenderPass(renderPass);
        }

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = subpassDependencies.size();
        renderPassInfo.pDependencies = subpassDependencies.data();
        renderPass = context.device.createRenderPass(renderPassInfo);
    }
}}
