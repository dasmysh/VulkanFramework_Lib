/**
 * @file   GraphicsPipeline.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Declaration of a Vulkan graphics pipeline object.
 */

#pragma once

#include "main.h"
#include "gfx/vk/wrappers/VulkanObjectWrapper.h"

#include <glm/vec2.hpp>

namespace vkfw_core::gfx {

    class Framebuffer; // NOLINT
    class Shader;

    class GraphicsPipeline final : public VulkanObjectWrapper<vk::UniquePipeline>
    {
    public:
        GraphicsPipeline(const LogicalDevice* device, std::string_view name,
                         const std::vector<std::shared_ptr<Shader>>& shaders, const glm::uvec2& size,
                         unsigned int numBlendAttachments);
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
        GraphicsPipeline(GraphicsPipeline&&) noexcept;
        GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept;
        ~GraphicsPipeline();

        void ResetShaders(const std::vector<std::shared_ptr<Shader>>& shaders);
        template<class Vertex> void ResetVertexInput() const;
        void ResetFramebuffer(const glm::uvec2& size, unsigned int numViewports, unsigned int numScissors) const;
        void CreatePipeline(bool keepState, vk::RenderPass renderPass, unsigned int subpass, vk::PipelineLayout pipelineLayout);

        [[nodiscard]] vk::Viewport& GetViewport(unsigned int idx) const
        {
            assert(m_state);
            return m_state->m_viewports[idx];
        }
        [[nodiscard]] vk::Rect2D& GetScissor(unsigned int idx) const
        {
            assert(m_state);
            return m_state->m_scissors[idx];
        }
        [[nodiscard]] vk::PipelineMultisampleStateCreateInfo& GetMultisampling() const
        {
            assert(m_state);
            return m_state->m_multisampling;
        }
        [[nodiscard]] vk::PipelineRasterizationStateCreateInfo& GetRasterizer() const
        {
            assert(m_state);
            return m_state->m_rasterizer;
        }
        [[nodiscard]] vk::PipelineDepthStencilStateCreateInfo& GetDepthStencil() const
        {
            assert(m_state);
            return m_state->m_depthStencil;
        }
        [[nodiscard]] vk::PipelineTessellationStateCreateInfo& GetTesselation() const
        {
            assert(m_state);
            return m_state->m_tesselation;
        }
        [[nodiscard]] vk::PipelineColorBlendAttachmentState& GetColorBlendAttachment(unsigned int idx) const
        {
            assert(m_state);
            return m_state->m_colorBlendAttachments[idx];
        }
        [[nodiscard]] vk::PipelineColorBlendStateCreateInfo& GetColorBlending() const
        {
            assert(m_state);
            return m_state->m_colorBlending;
        }
        [[nodiscard]] std::vector<vk::DynamicState>& GetDynamicStates() const
        {
            assert(m_state);
            return m_state->m_dynamicStates;
        }

    private:

        struct State final
        {
            /** Holds the information about the shaders used. */
            std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStageInfos;
            /** Holds the vertex input state. */
            vk::PipelineVertexInputStateCreateInfo m_vertexInputCreateInfo;
            /** Holds the input assembly state. */
            vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyCreateInfo;
            /** Holds the view-ports. */
            std::vector<vk::Viewport> m_viewports;
            /** Holds the scissors. */
            std::vector<vk::Rect2D> m_scissors;
            /** Holds the viewport state */
            vk::PipelineViewportStateCreateInfo m_viewportState;
            /** Holds the multi-sampling state. */
            vk::PipelineMultisampleStateCreateInfo m_multisampling;
            /** Holds the depth stencil state. */
            vk::PipelineDepthStencilStateCreateInfo m_depthStencil;
            /** Holds the rasterizer state. */
            vk::PipelineRasterizationStateCreateInfo m_rasterizer;
            /** Holds the tessellation state. */
            vk::PipelineTessellationStateCreateInfo m_tesselation;
            /** Holds the color blend attachments. */
            std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
            /** Holds the blend state. */
            vk::PipelineColorBlendStateCreateInfo m_colorBlending;
            /** Holds the dynamic states. */
            std::vector<vk::DynamicState> m_dynamicStates;
        };

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the shaders used in this pipeline. */
        std::vector<std::shared_ptr<Shader>> m_shaders;
        /** Holds the state. */
        std::unique_ptr<State> m_state;
    };

    template <class Vertex>
    void GraphicsPipeline::ResetVertexInput() const
    {
        m_state->m_vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo{ vk::PipelineVertexInputStateCreateFlags(), 1, &Vertex::m_bindingDescription,
            static_cast<std::uint32_t>(Vertex::m_attributeDescriptions.size()), Vertex::m_attributeDescriptions.data() };
    }
}

