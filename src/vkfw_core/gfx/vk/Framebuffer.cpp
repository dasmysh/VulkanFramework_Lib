/**
 * @file   Framebuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Implementation of a framebuffer object for Vulkan.
 */

#include "gfx/vk/Framebuffer.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/CommandBuffers.h"

namespace vku::gfx {

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size,
        const std::vector<vk::Image>& images, const vk::RenderPass& renderPass, const FramebufferDescriptor& desc,
        const std::vector<std::uint32_t>& queueFamilyIndices, vk::CommandBuffer cmdBuffer) :
        device_{ logicalDevice },
        size_{ size },
        renderPass_{ renderPass },
        desc_(desc),
        memoryGroup_{ logicalDevice, vk::MemoryPropertyFlags() },
        extImages_{ images },
        vkExternalAttachmentsImageView_{ desc.tex_.size() },
        queueFamilyIndices_{ queueFamilyIndices }
    {
        CreateImages(cmdBuffer);
        CreateFB();
    }

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size,
        const vk::RenderPass& renderPass, const FramebufferDescriptor& desc,
        const std::vector<std::uint32_t>& queueFamilyIndices, vk::CommandBuffer cmdBuffer) :
        Framebuffer(logicalDevice, size, std::vector<vk::Image>(), renderPass, desc, queueFamilyIndices, cmdBuffer)
    {
    }

    Framebuffer::Framebuffer(const Framebuffer& rhs) :
        Framebuffer(rhs.device_, rhs.size_, rhs.extImages_, rhs.renderPass_, rhs.desc_, rhs.queueFamilyIndices_)
    {
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
        device_{ rhs.device_ },
        size_{ rhs.size_ },
        renderPass_{ rhs.renderPass_ },
        desc_(std::move(rhs.desc_)),
        extImages_{ std::move(rhs.extImages_) },
        memoryGroup_{ std::move(rhs.memoryGroup_) },
        vkExternalAttachmentsImageView_{ std::move(rhs.vkExternalAttachmentsImageView_) },
        vkFramebuffer_{ std::move(rhs.vkFramebuffer_) },
        queueFamilyIndices_{ std::move(rhs.queueFamilyIndices_) }
    {
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
    {
        this->~Framebuffer();
        device_ = std::move(rhs.device_);
        size_ = std::move(rhs.size_);
        renderPass_ = std::move(rhs.renderPass_);
        desc_ = std::move(rhs.desc_);
        memoryGroup_ = std::move(rhs.memoryGroup_);
        extImages_ = std::move(rhs.extImages_);
        vkExternalAttachmentsImageView_ = std::move(rhs.vkExternalAttachmentsImageView_);
        vkFramebuffer_ = std::move(rhs.vkFramebuffer_);
        queueFamilyIndices_ = std::move(rhs.queueFamilyIndices_);
        return *this;
    }

    Framebuffer::~Framebuffer() = default;

    void Framebuffer::CreateImages(vk::CommandBuffer cmdBuffer)
    {
        for (auto i = extImages_.size(); i < desc_.tex_.size(); ++i) {
            memoryGroup_.AddTextureToGroup(desc_.tex_[i], glm::u32vec4(size_, 1, 1), 1, queueFamilyIndices_);
        }
        memoryGroup_.FinalizeDeviceGroup();

        vk::UniqueCommandBuffer ucmdBuffer;
        if (!cmdBuffer) {
            ucmdBuffer = CommandBuffers::beginSingleTimeSubmit(device_, 0);
        }
        for (auto i = 0U; i < memoryGroup_.GetImagesInGroup(); ++i)
            memoryGroup_.GetTexture(i)->TransitionLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal, cmdBuffer ? cmdBuffer : *ucmdBuffer);
        if (!cmdBuffer) {
            CommandBuffers::endSingleTimeSubmit(device_, *ucmdBuffer, 0, 0);
            device_->GetQueue(0, 0).waitIdle();
        }

    }

    void Framebuffer::CreateFB()
    {
        assert(desc_.type_ == vk::ImageViewType::e2D || desc_.type_ == vk::ImageViewType::eCube);
        assert(extImages_.size() + memoryGroup_.GetImagesInGroup() == desc_.tex_.size());
        std::uint32_t layerCount = 1;
        if (desc_.type_ == vk::ImageViewType::eCube) layerCount = 6;

        // TODO: handle cube textures. [3/20/2017 Sebastian Maisch]
        std::vector<vk::ImageView> attachments;
        for (auto i = 0U; i < extImages_.size(); ++i) {
            vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount };
            vk::ComponentMapping componentMapping{};
            vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), extImages_[i],
                desc_.type_, desc_.tex_[i].format_, vk::ComponentMapping(), subresourceRange };
            vkExternalAttachmentsImageView_[i] = device_->GetDevice().createImageViewUnique(imgViewCreateInfo);
            attachments.push_back(*vkExternalAttachmentsImageView_[i]);
        }

        for (auto i = 0U; i < memoryGroup_.GetImagesInGroup(); ++i) attachments.push_back(memoryGroup_.GetTexture(i)->GetImageView());

        vk::FramebufferCreateInfo fbCreateInfo{ vk::FramebufferCreateFlags(), renderPass_,
            static_cast<std::uint32_t>(desc_.tex_.size()), attachments.data(), size_.x, size_.y, layerCount };
        vkFramebuffer_ = device_->GetDevice().createFramebufferUnique(fbCreateInfo);
    }
}
