/**
 * @file   RenderPass.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.25
 *
 * @brief  Wrapper for the vulkan render pass object.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"
#include "gfx/vk/textures/Texture.h"

namespace vkfw_core::gfx {

    struct AttachmentDescriptor final
    {
        TextureDescriptor m_tex;
        vk::AttachmentLoadOp m_loadOp;
        vk::AttachmentStoreOp m_storeOp;
        vk::AttachmentLoadOp m_stencilLoadOp;
        vk::AttachmentStoreOp m_stencilStoreOp;
        vk::ImageLayout m_initialLayout;
        vk::ImageLayout m_finalLayout;

        AttachmentDescriptor(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store, vk::AttachmentLoadOp stencilLoad,
                             vk::AttachmentStoreOp stencilStore, vk::ImageLayout initialLayout,
                             vk::ImageLayout finalLayout, std::size_t bytesPP, vk::Format format,
                             vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        AttachmentDescriptor(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store, vk::AttachmentLoadOp stencilLoad,
                             vk::AttachmentStoreOp stencilStore, vk::ImageLayout initialLayout,
                             vk::ImageLayout finalLayout, const TextureDescriptor& textureDescriptor);

        void PopulateAttachmentInfo(std::uint32_t attachment,
                                    std::vector<vk::AttachmentDescription>& attachmentDescriptions,
                                    std::vector<vk::AttachmentReference>& colorAttachmentReferences,
                                    vk::AttachmentReference& dsAttachmentReference) const;
    };

    struct FramebufferDescriptor final
    {
        /** Holds the texture descriptors. */
        std::vector<AttachmentDescriptor> m_attachments;
        /** Holds the textures image type. */
        vk::ImageViewType m_type = vk::ImageViewType::e2D;
        /** Holds the pipeline binding point */
        vk::PipelineBindPoint m_bindingPoint = vk::PipelineBindPoint::eGraphics;
    };

    class RenderPass : public VulkanObjectWrapper<vk::UniqueRenderPass>
    {
    public:
        RenderPass() : VulkanObjectWrapper{nullptr, "", vk::UniqueRenderPass{}} {}
        RenderPass(vk::Device device, std::string_view name);
        RenderPass(vk::Device device, std::string_view name, const FramebufferDescriptor& desc);

        void Create(vk::Device device, const FramebufferDescriptor& desc);

        [[nodiscard]] const FramebufferDescriptor& GetDescriptor() const { return m_desc; }

    private:
        using VulkanObjectPrivateWrapper<vk::UniqueRenderPass>::SetHandle;
        void Create(vk::Device device);

        /** Holds the framebuffer descriptor. */
        FramebufferDescriptor m_desc;
    };
}
