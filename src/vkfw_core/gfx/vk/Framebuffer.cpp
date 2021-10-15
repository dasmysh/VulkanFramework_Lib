/**
 * @file   Framebuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Implementation of a framebuffer object for Vulkan.
 */

#include "gfx/vk/Framebuffer.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/PipelineBarriers.h"

namespace vkfw_core::gfx {

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, std::string_view name, const glm::uvec2& size,
                             const std::vector<vk::Image>& images, const RenderPass& renderPass,
                             const FramebufferDescriptor& desc, const std::vector<std::uint32_t>& queueFamilyIndices,
                             const CommandBuffer& cmdBuffer)
        : VulkanObjectWrapper{logicalDevice->GetHandle(), name, vk::UniqueFramebuffer{}}
        , m_device{logicalDevice}
        , m_size{size}
        , m_renderPass{&renderPass}
        , m_desc(desc)
        , m_memoryGroup{logicalDevice, fmt::format("Mem:{}", name), vk::MemoryPropertyFlags()}
        , m_extImages{std::move(images)}
        , m_queueFamilyIndices{std::move(queueFamilyIndices)}
    {
        for (std::size_t i = 0; i < m_extImages.size(); ++i) {
            m_extTextures.emplace_back(m_device, fmt::format("FBO:{}-ExtTex:{}", name, i), desc.m_tex[i],
                                       vk::ImageLayout::eUndefined, m_queueFamilyIndices);
            m_extTextures.back().InitializeExternalImage(m_extImages[i], glm::u32vec4(m_size, 1, 1), 1, false);
        }

        CreateImages(cmdBuffer);
        CreateFB();
    }

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, std::string_view name, const glm::uvec2& size,
                             const RenderPass& renderPass, const FramebufferDescriptor& desc,
                             const std::vector<std::uint32_t>& queueFamilyIndices, const CommandBuffer& cmdBuffer)
        : Framebuffer(logicalDevice, name, size, std::vector<vk::Image>(), renderPass, desc, queueFamilyIndices,
                      cmdBuffer)
    {
    }

    Framebuffer::Framebuffer(const Framebuffer& rhs)
        : Framebuffer(rhs.m_device, fmt::format("{}-Copy", GetName()), rhs.m_size, rhs.m_extImages, *rhs.m_renderPass,
                      rhs.m_desc, rhs.m_queueFamilyIndices)
    {
    }

    Framebuffer& Framebuffer::operator=(const Framebuffer& rhs)
    {
        if (this != &rhs) {
            auto tmp{rhs};
            std::swap(*this, tmp);
        }
        return *this;
    }

    Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept
        : VulkanObjectWrapper{std::move(rhs)}
        , m_device{rhs.m_device}
        , m_size{rhs.m_size}
        , m_renderPass{rhs.m_renderPass}
        , m_desc(std::move(rhs.m_desc))
        , m_memoryGroup{std::move(rhs.m_memoryGroup)}
        , m_extTextures{std::move(rhs.m_extTextures)}
        , m_extImages{std::move(rhs.m_extImages)}
        , m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
    {
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
    {
        this->~Framebuffer();
        VulkanObjectWrapper::operator=(std::move(rhs));
        m_device = rhs.m_device;
        m_size = rhs.m_size;
        m_renderPass = rhs.m_renderPass;
        m_desc = std::move(rhs.m_desc);
        m_memoryGroup = std::move(rhs.m_memoryGroup);
        m_extTextures = std::move(rhs.m_extTextures);
        m_extImages = std::move(rhs.m_extImages);
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        return *this;
    }

    Framebuffer::~Framebuffer() = default;

    Texture& Framebuffer::GetTexture(std::size_t index)
    {
        assert(index < m_desc.m_tex.size());
        if (index < m_extTextures.size()) {
            return m_extTextures[index];
        } else {
            return *m_memoryGroup.GetTexture(static_cast<unsigned int>(index - m_extTextures.size()));
        }
    }

    void Framebuffer::CreateImages(const CommandBuffer& cmdBuffer)
    {
        std::vector<vk::ImageLayout> imageLayouts;
        imageLayouts.resize(m_desc.m_tex.size() - m_extTextures.size());
        for (auto i = m_extTextures.size(); i < m_desc.m_tex.size(); ++i) {
            imageLayouts[i - m_extTextures.size()] = IsDepthStencilFormat(m_desc.m_tex[i].m_format)
                                                         ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                                         : vk::ImageLayout::eColorAttachmentOptimal;
            m_memoryGroup.AddTextureToGroup(fmt::format("FBO:{}-Tex{}", GetName(), i), m_desc.m_tex[i],
                                            vk::ImageLayout::eUndefined, glm::u32vec4(m_size, 1, 1), 1,
                                            m_queueFamilyIndices);
        }
        m_memoryGroup.FinalizeDeviceGroup();

        CommandBuffer ucmdBuffer;
        if (!cmdBuffer) {
            ucmdBuffer = CommandBuffer::beginSingleTimeSubmit(
                m_device, fmt::format("FBO:{} CreateImagesCmdBuffer", GetName()),
                fmt::format("FBO:{} CreateImages", GetName()), m_device->GetCommandPool(0));
        }
        PipelineBarrier barrier{m_device, vk::PipelineStageFlagBits::eColorAttachmentOutput};
        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            vk::AccessFlags access = vk::AccessFlagBits::eColorAttachmentWrite;
            vk::PipelineStageFlags pipelineStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            if (IsDepthStencilFormat(m_desc.m_tex[i].m_format)) {
                access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                pipelineStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            }

            auto accessor = m_memoryGroup.GetTexture(i)->GetAccess();
            [[maybe_unused]] auto dummy = accessor.Get(access, pipelineStage, imageLayouts[i], barrier);
            // m_memoryGroup.GetTexture(i)->TransitionLayout(imageLayouts[i], cmdBuffer ? cmdBuffer : ucmdBuffer);
        }
        barrier.Record(cmdBuffer ? cmdBuffer : ucmdBuffer);
        if (!cmdBuffer) {
            auto& queue = m_device->GetQueue(0, 0);
            CommandBuffer::endSingleTimeSubmit(queue, ucmdBuffer);
            queue.WaitIdle();
        }
    }

    void Framebuffer::CreateFB()
    {
        assert(m_desc.m_type == vk::ImageViewType::e2D || m_desc.m_type == vk::ImageViewType::eCube);
        assert(m_extTextures.size() + m_memoryGroup.GetImagesInGroup() == m_desc.m_tex.size());
        constexpr std::uint32_t CUBE_MAP_LAYERS = 6;
        std::uint32_t layerCount = 1;
        if (m_desc.m_type == vk::ImageViewType::eCube) { layerCount = CUBE_MAP_LAYERS; }

        // TODO: handle cube textures. [3/20/2017 Sebastian Maisch]
        std::vector<vk::ImageView> attachments;
        for (auto& tex : m_extTextures) {
            tex.InitializeImageView();
            attachments.push_back(tex.GetImageView().GetHandle());
        }

        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            attachments.push_back(m_memoryGroup.GetTexture(i)->GetImageView().GetHandle());
        }

        vk::FramebufferCreateInfo fbCreateInfo{vk::FramebufferCreateFlags(),
                                               m_renderPass->GetHandle(),
                                               static_cast<std::uint32_t>(m_desc.m_tex.size()),
                                               attachments.data(),
                                               m_size.x,
                                               m_size.y,
                                               layerCount};
        SetHandle(m_device->GetHandle(), m_device->GetHandle().createFramebufferUnique(fbCreateInfo));
    }

    bool Framebuffer::IsDepthStencilFormat(vk::Format format)
    {
        if (format == vk::Format::eD16Unorm) {
            return true;
        } else if (format == vk::Format::eX8D24UnormPack32) {
            return true;
        } else if (format == vk::Format::eD32Sfloat) {
            return true;
        } else if (format == vk::Format::eS8Uint) {
            return true;
        } else if (format == vk::Format::eD16UnormS8Uint) {
            return true;
        } else if (format == vk::Format::eD24UnormS8Uint) {
            return true;
        } else if (format == vk::Format::eD32SfloatS8Uint) {
            return true;
        }
        return false;
    }
}
