/**
 * @file   RenderPass.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.18
 *
 * @brief  Implementation of the RenderPass wrapper class.
 */

#include "gfx/vk/wrappers/RenderPass.h"
#include "gfx/vk/Framebuffer.h"

namespace vkfw_core::gfx {

    AttachmentDescriptor::AttachmentDescriptor(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store,
                                               vk::AttachmentLoadOp stencilLoad, vk::AttachmentStoreOp stencilStore,
                                               vk::ImageLayout initialLayout, vk::ImageLayout finalLayout,
                                               std::size_t bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
        : m_tex{bytesPP, format, samples}
        , m_loadOp{load}
        , m_storeOp{store}
        , m_stencilLoadOp{stencilLoad}
        , m_stencilStoreOp{stencilStore}
        , m_initialLayout{initialLayout}
        , m_finalLayout{finalLayout}
    {
    }

    AttachmentDescriptor::AttachmentDescriptor(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store,
                                               vk::AttachmentLoadOp stencilLoad, vk::AttachmentStoreOp stencilStore,
                                               vk::ImageLayout initialLayout, vk::ImageLayout finalLayout,
                                               const TextureDescriptor& textureDescriptor)
        : m_tex{textureDescriptor}
        , m_loadOp{load}
        , m_storeOp{store}
        , m_stencilLoadOp{stencilLoad}
        , m_stencilStoreOp{stencilStore}
        , m_initialLayout{initialLayout}
        , m_finalLayout{finalLayout}
    {
    }

    void AttachmentDescriptor::PopulateAttachmentInfo(std::uint32_t attachment,
                                                      std::vector<vk::AttachmentDescription>& attachmentDescriptions,
                                                      std::vector<vk::AttachmentReference>& colorAttachmentReferences,
                                                      vk::AttachmentReference& dsAttachmentReference) const
    {
        attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags{}, m_tex.m_format, m_tex.m_samples, m_loadOp,
                                            m_storeOp, m_stencilLoadOp, m_stencilStoreOp, m_initialLayout,
                                            m_finalLayout);
        if (Framebuffer::IsAnyDepthOrStencilFormat(m_tex.m_format)) {
            dsAttachmentReference = vk::AttachmentReference{static_cast<std::uint32_t>(attachment),
                                                            Framebuffer::GetFittingAttachmentLayout(m_tex.m_format)};
        } else {
            colorAttachmentReferences.emplace_back(static_cast<std::uint32_t>(attachment),
                                                   Framebuffer::GetFittingAttachmentLayout(m_tex.m_format));
        }
    }

    RenderPass::RenderPass(vk::Device device, std::string_view name)
        : VulkanObjectWrapper{device, name, vk::UniqueRenderPass{}}
    {
    }

    RenderPass::RenderPass(vk::Device device, std::string_view name, const FramebufferDescriptor& desc)
        : RenderPass{device, name}
    {
        m_desc = desc;
        Create(device);
    }

    void RenderPass::Create(vk::Device device, const FramebufferDescriptor& desc)
    {
        m_desc = desc;
        Create(device);
    }

    void RenderPass::Create(vk::Device device)
    {
        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        attachmentDescriptions.reserve(m_desc.m_attachments.size());
        std::vector<vk::AttachmentReference> colorAttachmentReferences;
        vk::AttachmentReference dsAttachmentReference;
        for (std::size_t i = 0; i < m_desc.m_attachments.size(); ++i) {
            const auto& attachmentDesc = m_desc.m_attachments[i];
            attachmentDesc.PopulateAttachmentInfo(static_cast<uint32_t>(i), attachmentDescriptions,
                                                  colorAttachmentReferences, dsAttachmentReference);
        }

        // TODO: support multiple subpasses in the future.
        vk::SubpassDescription subPass{
            vk::SubpassDescriptionFlags{}, m_desc.m_bindingPoint, {}, colorAttachmentReferences, {},
            &dsAttachmentReference};

        // TODO: access flags could be restricted ...
        vk::SubpassDependency dependency{VK_SUBPASS_EXTERNAL,
                                         0,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                         vk::AccessFlags(),
                                         vk::AccessFlagBits::eColorAttachmentRead
                                             | vk::AccessFlagBits::eColorAttachmentWrite};
        vk::RenderPassCreateInfo renderPassInfo{vk::RenderPassCreateFlags{}, attachmentDescriptions, subPass,
                                                dependency};

        ResetHandle(device, device.createRenderPassUnique(renderPassInfo));
    }
}
