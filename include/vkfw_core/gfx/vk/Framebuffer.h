/**
 * @file   Framebuffer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a framebuffer object for Vulkan.
 */

#pragma once

#include "main.h"
#include "gfx/vk/textures/DeviceTexture.h"
#include "memory/DeviceMemoryGroup.h"
#include "gfx/vk/wrappers/RenderPass.h"

namespace vkfw_core::gfx {

    class LogicalDevice;
    class DescriptorSet;

    class Framebuffer final : public VulkanObjectPrivateWrapper<vk::UniqueFramebuffer>
    {
    public:
        Framebuffer(const LogicalDevice* logicalDevice, std::string_view name, const glm::uvec2& size,
                    const std::vector<vk::Image>& images, const RenderPass& renderPass,
                    const FramebufferDescriptor& desc,
                    const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{},
                    const CommandBuffer& cmdBuffer = CommandBuffer{});
        Framebuffer(const LogicalDevice* logicalDevice, std::string_view name, const glm::uvec2& size,
                    const RenderPass& renderPass, const FramebufferDescriptor& desc,
                    const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{},
                    const CommandBuffer& cmdBuffer = CommandBuffer{});
        Framebuffer(const Framebuffer&);
        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(const Framebuffer&);
        Framebuffer& operator=(Framebuffer&&) noexcept;
        ~Framebuffer();

        [[nodiscard]] glm::uvec2 GetSize() const { return m_size; }
        [[nodiscard]] unsigned int GetWidth() const { return m_size.x; }
        [[nodiscard]] unsigned int GetHeight() const { return m_size.y; }
        [[nodiscard]] const FramebufferDescriptor& GetDescriptor() const { return m_desc; }
        [[nodiscard]] Texture& GetTexture(std::size_t index);

        [[nodiscard]] Framebuffer
        CreateWithSameImages(std::string_view name, const FramebufferDescriptor& desc,
                             const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{},
                             const CommandBuffer& cmdBuffer = CommandBuffer{}) const;

        void BeginRenderPass(const CommandBuffer& cmdBuffer, const RenderPass& renderPass,
                             std::span<DescriptorSet*> descriptorSets, const vk::Rect2D& renderArea,
                             std::span<vk::ClearValue> clearColor, vk::SubpassContents subpassContents);

        static [[nodiscard]] bool IsAnyDepthOrStencilFormat(vk::Format format);
        static [[nodiscard]] bool IsDepthStencilFormat(vk::Format format);
        static [[nodiscard]] bool IsStencilFormat(vk::Format format);
        static [[nodiscard]] bool IsDepthFormat(vk::Format format);

        static [[nodiscard]] vk::ImageLayout GetFittingAttachmentLayout(vk::Format format);
        static [[nodiscard]] vk::AccessFlags GetFittingAttachmentAccessFlags(vk::Format format);
        static [[nodiscard]] vk::PipelineStageFlags GetFittingAttachmentPipelineStage(vk::Format format);

    private:
        void CreateImages(const CommandBuffer& cmdBuffer);
        void CreateFB();

        /** Holds the logical device. */
        const LogicalDevice* m_device;
        /** Holds the framebuffer size. */
        glm::uvec2 m_size;
        /** Holds the render pass. */
        const RenderPass* m_renderPass;
        /** Holds the framebuffer descriptor. */
        FramebufferDescriptor m_desc;
        /** Holds the device memory group for the owned images. */
        DeviceMemoryGroup m_memoryGroup;
        /** Holds the externally owned images in this framebuffer. */
        std::vector<Texture> m_extTextures;
        std::vector<vk::Image> m_extImages;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> m_queueFamilyIndices;
        /** Pipeline barrier for using this framebuffer. */
        gfx::PipelineBarrier m_barrier;
    };
}
