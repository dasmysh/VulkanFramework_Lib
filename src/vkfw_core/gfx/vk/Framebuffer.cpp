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

namespace vkfw_core::gfx {

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size,
        std::vector<vk::Image> images, const vk::RenderPass& renderPass, const FramebufferDescriptor& desc,
        std::vector<std::uint32_t> queueFamilyIndices, vk::CommandBuffer cmdBuffer) :
        m_device{ logicalDevice },
        m_size{ size },
        m_renderPass{ renderPass },
        m_desc(desc),
        m_memoryGroup{ logicalDevice, vk::MemoryPropertyFlags() },
        m_extImages{ std::move(images) },
        m_vkExternalAttachmentsImageView{ desc.m_tex.size() },
        m_queueFamilyIndices{ std::move(queueFamilyIndices) }
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
        Framebuffer(rhs.m_device, rhs.m_size, rhs.m_extImages, rhs.m_renderPass, rhs.m_desc, rhs.m_queueFamilyIndices)
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

    Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept
        : m_device{rhs.m_device},
          m_size{rhs.m_size},
          m_renderPass{rhs.m_renderPass},
          m_desc(std::move(rhs.m_desc)),
          m_memoryGroup{std::move(rhs.m_memoryGroup)},
          m_extImages{std::move(rhs.m_extImages)},
          m_vkExternalAttachmentsImageView{std::move(rhs.m_vkExternalAttachmentsImageView)},
          m_vkFramebuffer{std::move(rhs.m_vkFramebuffer)},
          m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
    {
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
    {
        this->~Framebuffer();
        m_device = rhs.m_device;
        m_size = rhs.m_size;
        m_renderPass = rhs.m_renderPass;
        m_desc = std::move(rhs.m_desc);
        m_memoryGroup = std::move(rhs.m_memoryGroup);
        m_extImages = std::move(rhs.m_extImages);
        m_vkExternalAttachmentsImageView = std::move(rhs.m_vkExternalAttachmentsImageView);
        m_vkFramebuffer = std::move(rhs.m_vkFramebuffer);
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        return *this;
    }

    Framebuffer::~Framebuffer() = default;

    void Framebuffer::CreateImages(vk::CommandBuffer cmdBuffer)
    {
        for (auto i = m_extImages.size(); i < m_desc.m_tex.size(); ++i) {
            m_memoryGroup.AddTextureToGroup(m_desc.m_tex[i], glm::u32vec4(m_size, 1, 1), 1, m_queueFamilyIndices);
        }
        m_memoryGroup.FinalizeDeviceGroup();

        vk::UniqueCommandBuffer ucmdBuffer;
        if (!cmdBuffer) { ucmdBuffer = CommandBuffers::beginSingleTimeSubmit(m_device, 0); }
        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            m_memoryGroup.GetTexture(i)->TransitionLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                         cmdBuffer ? cmdBuffer : *ucmdBuffer);
        }
        if (!cmdBuffer) {
            CommandBuffers::endSingleTimeSubmit(m_device, *ucmdBuffer, 0, 0);
            m_device->GetQueue(0, 0).waitIdle();
        }

    }

    void Framebuffer::CreateFB()
    {
        assert(m_desc.m_type == vk::ImageViewType::e2D || m_desc.m_type == vk::ImageViewType::eCube);
        assert(m_extImages.size() + m_memoryGroup.GetImagesInGroup() == m_desc.m_tex.size());
        constexpr std::uint32_t CUBE_MAP_LAYERS = 6;
        std::uint32_t layerCount = 1;
        if (m_desc.m_type == vk::ImageViewType::eCube) { layerCount = CUBE_MAP_LAYERS; }

        // TODO: handle cube textures. [3/20/2017 Sebastian Maisch]
        std::vector<vk::ImageView> attachments;
        for (auto i = 0U; i < m_extImages.size(); ++i) {
            vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount };
            vk::ImageViewCreateInfo imgViewCreateInfo{ vk::ImageViewCreateFlags(), m_extImages[i],
                m_desc.m_type, m_desc.m_tex[i].m_format, vk::ComponentMapping(), subresourceRange };
            m_vkExternalAttachmentsImageView[i] = m_device->GetDevice().createImageViewUnique(imgViewCreateInfo);
            attachments.push_back(*m_vkExternalAttachmentsImageView[i]);
        }

        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            attachments.push_back(m_memoryGroup.GetTexture(i)->GetImageView());
        }

        vk::FramebufferCreateInfo fbCreateInfo{ vk::FramebufferCreateFlags(), m_renderPass,
                                               static_cast<std::uint32_t>(m_desc.m_tex.size()),
                                               attachments.data(),
                                               m_size.x,
                                               m_size.y,
                                               layerCount};
        m_vkFramebuffer = m_device->GetDevice().createFramebufferUnique(fbCreateInfo);
    }
}
