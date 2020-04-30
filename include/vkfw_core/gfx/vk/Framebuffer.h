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

namespace vkfw_core::gfx {

    class LogicalDevice;

    struct FramebufferDescriptor final
    {
        /** Holds the texture descriptors. */
        std::vector<TextureDescriptor> m_tex;
        /** Holds the textures image type. */
        vk::ImageViewType m_type = vk::ImageViewType::e2D;
    };

    class Framebuffer final
    {
    public:
        Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size, std::vector<vk::Image> images,
            const vk::RenderPass& renderPass, const FramebufferDescriptor& desc,
            std::vector<std::uint32_t> queueFamilyIndices = std::vector<std::uint32_t>{},
            vk::CommandBuffer cmdBuffer = vk::CommandBuffer());
        Framebuffer(const LogicalDevice* logicalDevice, const glm::uvec2& size, const vk::RenderPass& renderPass,
            const FramebufferDescriptor& desc, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{},
            vk::CommandBuffer cmdBuffer = vk::CommandBuffer());
        Framebuffer(const Framebuffer&);
        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(const Framebuffer&);
        Framebuffer& operator=(Framebuffer&&) noexcept;
        ~Framebuffer();

        [[nodiscard]] glm::uvec2 GetSize() const { return m_size; }
        [[nodiscard]] unsigned int GetWidth() const { return m_size.x; }
        [[nodiscard]] unsigned int GetHeight() const { return m_size.y; }
        [[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return *m_vkFramebuffer; }
        [[nodiscard]] const FramebufferDescriptor& GetDescriptor() const { return m_desc; }
        [[nodiscard]] Texture& GetTexture(std::size_t index);

    private:
        void CreateImages(vk::CommandBuffer cmdBuffer);
        void CreateFB();
        bool IsDepthStencilFormat(vk::Format format);

        /** Holds the logical device. */
        const LogicalDevice* m_device;
        /** Holds the framebuffer size. */
        glm::uvec2 m_size;
        /** Holds the render pass. */
        vk::RenderPass m_renderPass;
        /** Holds the framebuffer descriptor. */
        FramebufferDescriptor m_desc;
        /** Holds the device memory group for the owned images. */
        DeviceMemoryGroup m_memoryGroup;
        /** Holds the externally owned images in this framebuffer. */
        std::vector<Texture> m_extImages;
        std::vector<vk::Image> m_vkExtImages;
        /** Holds the Vulkan framebuffer object. */
        vk::UniqueFramebuffer m_vkFramebuffer;
        /** Holds the queue family indices. */
        std::vector<std::uint32_t> m_queueFamilyIndices;
    };
}
