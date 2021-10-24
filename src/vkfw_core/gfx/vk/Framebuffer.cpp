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
#include "gfx/vk/wrappers/DescriptorSet.h"

namespace vkfw_core::gfx {

    Framebuffer::Framebuffer(const LogicalDevice* logicalDevice, std::string_view name, const glm::uvec2& size,
                             const std::vector<vk::Image>& images, const RenderPass& renderPass,
                             const FramebufferDescriptor& desc, const std::vector<std::uint32_t>& queueFamilyIndices,
                             const CommandBuffer& cmdBuffer)
        : VulkanObjectPrivateWrapper{logicalDevice->GetHandle(), name, vk::UniqueFramebuffer{}}
        , m_device{logicalDevice}
        , m_size{size}
        , m_renderPass{&renderPass}
        , m_desc(desc)
        , m_memoryGroup{logicalDevice, fmt::format("Mem:{}", name), vk::MemoryPropertyFlags()}
        , m_extImages{images}
        , m_queueFamilyIndices{queueFamilyIndices}
        , m_barrier{m_device}
    {
        for (std::size_t i = 0; i < m_extImages.size(); ++i) {
            m_extTextures.emplace_back(m_device, fmt::format("FBO:{}-ExtTex:{}", name, i), desc.m_attachments[i].m_tex,
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
        : VulkanObjectPrivateWrapper{std::move(rhs)}
        , m_device{rhs.m_device}
        , m_size{rhs.m_size}
        , m_renderPass{rhs.m_renderPass}
        , m_desc(std::move(rhs.m_desc))
        , m_memoryGroup{std::move(rhs.m_memoryGroup)}
        , m_extTextures{std::move(rhs.m_extTextures)}
        , m_extImages{std::move(rhs.m_extImages)}
        , m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
        , m_barrier{std::move(rhs.m_barrier)}
    {
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
    {
        this->~Framebuffer();
        VulkanObjectPrivateWrapper::operator=(std::move(rhs));
        m_device = rhs.m_device;
        m_size = rhs.m_size;
        m_renderPass = rhs.m_renderPass;
        m_desc = std::move(rhs.m_desc);
        m_memoryGroup = std::move(rhs.m_memoryGroup);
        m_extTextures = std::move(rhs.m_extTextures);
        m_extImages = std::move(rhs.m_extImages);
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        m_barrier = std::move(rhs.m_barrier);
        return *this;
    }

    Framebuffer::~Framebuffer() = default;

    Texture& Framebuffer::GetTexture(std::size_t index)
    {
        assert(index < m_desc.m_attachments.size());
        if (index < m_extTextures.size()) {
            return m_extTextures[index];
        } else {
            return *m_memoryGroup.GetTexture(static_cast<unsigned int>(index - m_extTextures.size()));
        }
    }

    void Framebuffer::CreateImages(const CommandBuffer& cmdBuffer)
    {
        std::vector<vk::ImageLayout> imageLayouts;
        imageLayouts.resize(m_desc.m_attachments.size() - m_extTextures.size());
        for (auto i = m_extTextures.size(); i < m_desc.m_attachments.size(); ++i) {
            imageLayouts[i - m_extTextures.size()] = GetFittingAttachmentLayout(m_desc.m_attachments[i].m_tex.m_format);
            m_memoryGroup.AddTextureToGroup(fmt::format("FBO:{}-Tex{}", GetName(), i), m_desc.m_attachments[i].m_tex,
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
            vk::AccessFlags access = GetFittingAttachmentAccessFlags(m_desc.m_attachments[i].m_tex.m_format);
            vk::PipelineStageFlags pipelineStage =
                GetFittingAttachmentPipelineStage(m_desc.m_attachments[i].m_tex.m_format);
            m_memoryGroup.GetTexture(i)->AccessBarrier(access, pipelineStage, imageLayouts[i], barrier);
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
        assert(m_extTextures.size() + m_memoryGroup.GetImagesInGroup() == m_desc.m_attachments.size());
        constexpr std::uint32_t CUBE_MAP_LAYERS = 6;
        std::uint32_t layerCount = 1;
        if (m_desc.m_type == vk::ImageViewType::eCube) { layerCount = CUBE_MAP_LAYERS; }

        // TODO: handle cube textures. [3/20/2017 Sebastian Maisch]
        std::vector<vk::ImageView> attachments;
        for (auto& tex : m_extTextures) {
            tex.InitializeImageView();
            auto descriptorIndex = attachments.size();
            vk::AccessFlags access =
                GetFittingAttachmentAccessFlags(m_desc.m_attachments[descriptorIndex].m_tex.m_format);
            vk::PipelineStageFlags pipelineStage =
                GetFittingAttachmentPipelineStage(m_desc.m_attachments[descriptorIndex].m_tex.m_format);
            vk::ImageLayout layout =
                m_desc.m_attachments[descriptorIndex].m_initialLayout == vk::ImageLayout::eUndefined
                    ? tex.GetImageLayout()
                    : m_desc.m_attachments[descriptorIndex].m_initialLayout;
            attachments.push_back(tex.GetImageView(access, pipelineStage, layout, m_barrier).GetHandle());
        }
        attachments.reserve(m_desc.m_attachments.size());

        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            auto descriptorIndex = attachments.size();

            vk::AccessFlags access =
                GetFittingAttachmentAccessFlags(m_desc.m_attachments[descriptorIndex].m_tex.m_format);
            vk::PipelineStageFlags pipelineStage =
                GetFittingAttachmentPipelineStage(m_desc.m_attachments[descriptorIndex].m_tex.m_format);
            vk::ImageLayout layout =
                m_desc.m_attachments[descriptorIndex].m_initialLayout == vk::ImageLayout::eUndefined
                    ? m_memoryGroup.GetTexture(i)->GetImageLayout()
                    : m_desc.m_attachments[descriptorIndex].m_initialLayout;
            attachments.push_back(
                m_memoryGroup.GetTexture(i)->GetImageView(access, pipelineStage, layout, m_barrier).GetHandle());
        }

        vk::FramebufferCreateInfo fbCreateInfo{vk::FramebufferCreateFlags(),
                                               m_renderPass->GetHandle(),
                                               static_cast<std::uint32_t>(m_desc.m_attachments.size()),
                                               attachments.data(),
                                               m_size.x,
                                               m_size.y,
                                               layerCount};

        SetHandle(m_device->GetHandle(), m_device->GetHandle().createFramebufferUnique(fbCreateInfo));
    }

    Framebuffer Framebuffer::CreateWithSameImages(std::string_view name, const FramebufferDescriptor& desc,
                                                  const std::vector<std::uint32_t>& queueFamilyIndices,
                                                  const CommandBuffer& cmdBuffer) const
    {
        std::vector<vk::Image> images;
        for (const auto& tex : m_extTextures) { images.push_back(tex.GetAccessNoBarrier()); }

        for (auto i = 0U; i < m_memoryGroup.GetImagesInGroup(); ++i) {
            images.push_back(m_memoryGroup.GetTexture(i)->GetAccessNoBarrier());
        }
        return Framebuffer(m_device, name, m_size, images, *m_renderPass, desc, queueFamilyIndices, cmdBuffer);
    }

    void Framebuffer::BeginRenderPass(const CommandBuffer& cmdBuffer, const RenderPass& renderPass,
                                      std::span<DescriptorSet*> descriptorSets, const vk::Rect2D& renderArea,
                                      std::span<vk::ClearValue> clearColor, vk::SubpassContents subpassContents)
    {
        m_barrier.Record(cmdBuffer);
        for (auto descriptorSet : descriptorSets) { descriptorSet->BindBarrier(cmdBuffer); }

        vk::RenderPassBeginInfo renderPassBeginInfo{renderPass.GetHandle(), GetHandle(), renderArea,
                                                    static_cast<std::uint32_t>(clearColor.size()), clearColor.data()};
        cmdBuffer.GetHandle().beginRenderPass(renderPassBeginInfo, subpassContents);
    }

    bool Framebuffer::IsAnyDepthOrStencilFormat(vk::Format format)
    {
        return IsDepthFormat(format) || IsStencilFormat(format) || IsDepthStencilFormat(format);
    }

    bool Framebuffer::IsDepthStencilFormat(vk::Format format)
    {
        if (format == vk::Format::eD16UnormS8Uint) {
            return true;
        } else if (format == vk::Format::eD24UnormS8Uint) {
            return true;
        } else if (format == vk::Format::eD32SfloatS8Uint) {
            return true;
        }
        return false;
    }

    bool Framebuffer::IsStencilFormat(vk::Format format)
    {
        if (format == vk::Format::eS8Uint) {
            return true;
        }
        return false;
    }

    bool Framebuffer::IsDepthFormat(vk::Format format)
    {
        if (format == vk::Format::eD16Unorm) {
            return true;
        } else if (format == vk::Format::eX8D24UnormPack32) {
            return true;
        } else if (format == vk::Format::eD32Sfloat) {
            return true;
        }
        return false;
    }

    vk::ImageLayout Framebuffer::GetFittingAttachmentLayout(vk::Format format)
    {
        // at this point feature separateDepthStencilLayouts is not enabled -> all layouts that fit depth or stencil are d/s layouts.
        // if (IsDepthFormat(format)) {
        //     return vk::ImageLayout::eDepthAttachmentOptimal;
        // } else if (IsStencilFormat(format)) {
        //     return vk::ImageLayout::eStencilAttachmentOptimal;
        // } else if (IsDepthStencilFormat(format)) {
        //     return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        // }
        if (IsAnyDepthOrStencilFormat(format)) {
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        return vk::ImageLayout::eColorAttachmentOptimal;
    }

    vk::AccessFlags Framebuffer::GetFittingAttachmentAccessFlags(vk::Format format)
    {
        if (IsAnyDepthOrStencilFormat(format)) {
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
        return vk::AccessFlagBits::eColorAttachmentWrite;
    }

    vk::PipelineStageFlags Framebuffer::GetFittingAttachmentPipelineStage(vk::Format format)
    {
        if (IsAnyDepthOrStencilFormat(format)) {
            return vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    }
}
