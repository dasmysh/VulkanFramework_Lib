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

    Framebuffer::Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const std::vector<vk::Image>& images, const vk::RenderPass& renderPass, const FramebufferDescriptor& desc) :
        logicalDevice_{ logicalDevice },
        size_{ size },
        renderPass_{ renderPass },
        desc_( desc ),
        images_{ images },
        imageOwnership_{ false },
        vkAttachmentsImageView_{ desc.tex_.size() }
    {
        CreateFB();
    }

    Framebuffer::Framebuffer(LogicalDevice* logicalDevice, const glm::uvec2& size, const vk::RenderPass& renderPass, const FramebufferDescriptor& desc) :
        logicalDevice_{ logicalDevice },
        size_{ size },
        renderPass_{ renderPass },
        desc_( desc ),
        vkAttachmentsImageView_{ desc.tex_.size() }
    {
        CreateImages();
        CreateFB();
    }

    Framebuffer::Framebuffer(const Framebuffer& rhs) :
        logicalDevice_{ rhs.logicalDevice_ },
        size_ { rhs.size_ },
        renderPass_{ rhs.renderPass_ },
        desc_( rhs.desc_ ),
        imageOwnership_{ rhs.imageOwnership_ },
        vkAttachmentsImageView_{ desc_.tex_.size() }
    {
        if (imageOwnership_) CreateImages();
        else images_ = rhs.images_;
        CreateFB();
    }

    Framebuffer& Framebuffer::operator=(const Framebuffer& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept :
        logicalDevice_{ rhs.logicalDevice_ },
        size_{ rhs.size_ },
        renderPass_{ rhs.renderPass_ },
        desc_( std::move(rhs.desc_) ),
        images_{ std::move(rhs.images_) },
        imageOwnership_{ rhs.imageOwnership_ },
        vkAttachmentsImageView_{ std::move(rhs.vkAttachmentsImageView_) },
        vkFramebuffer_{ std::move(rhs.vkFramebuffer_) }
    {
        rhs.vkFramebuffer_ = vk::Framebuffer();
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~Framebuffer();
            logicalDevice_ = std::move(rhs.logicalDevice_);
            size_ = std::move(rhs.size_);
            renderPass_ = std::move(rhs.renderPass_);
            desc_ = std::move(rhs.desc_);
            images_ = std::move(rhs.images_);
            imageOwnership_ = std::move(rhs.imageOwnership_);
            vkAttachmentsImageView_ = std::move(rhs.vkAttachmentsImageView_);
            vkFramebuffer_ = std::move(rhs.vkFramebuffer_);
            rhs.vkFramebuffer_ = vk::Framebuffer();
        }
        return *this;
    }

    Framebuffer::~Framebuffer()
    {
        if (vkFramebuffer_) logicalDevice_->GetDevice().destroyFramebuffer(vkFramebuffer_);
        vkFramebuffer_ = vk::Framebuffer();

        for (auto& imgView : vkAttachmentsImageView_) {
            if (imgView) logicalDevice_->GetDevice().destroyImageView(imgView);
            imgView = vk::ImageView();
        }

        if (imageOwnership_) {
            // TODO: destroy images. [10/27/2016 Sebastian Maisch]
        }
    }

    void Framebuffer::CreateImages()
    {
        // TODO: Create images... [10/27/2016 Sebastian Maisch]
    }

    void Framebuffer::CreateFB()
    {
        assert(desc_.type_ == vk::ImageViewType::e2D || desc_.type_ == vk::ImageViewType::eCube);
        assert(images_.size() == desc_.tex_.size());
        uint32_t layerCount = 1;
        if (desc_.type_ == vk::ImageViewType::eCube) layerCount = 6;

        // TODO: handle depth/stencil buffers... [10/27/2016 Sebastian Maisch]
        // TODO: also use external texture objects? [10/27/2016 Sebastian Maisch]
        for (auto i = 0U; i < desc_.tex_.size(); ++i) {
            vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount };
            vk::ComponentMapping componentMapping{};
            vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), images_[i], desc_.type_, desc_.tex_[i].format_, componentMapping, subresourceRange };
            vkAttachmentsImageView_[i] = logicalDevice_->GetDevice().createImageView(imgViewCreateInfo);
        }

        vk::FramebufferCreateInfo fbCreateInfo{ vk::FramebufferCreateFlags(), renderPass_,
            static_cast<uint32_t>(desc_.tex_.size()), vkAttachmentsImageView_.data(), size_.x, size_.y, layerCount };
        vkFramebuffer_ = logicalDevice_->GetDevice().createFramebuffer(fbCreateInfo);
    }
}}
