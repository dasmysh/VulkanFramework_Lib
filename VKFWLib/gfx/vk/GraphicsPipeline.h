/**
 * @file   GraphicsPipeline.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Declaration of a vulkan graphics pipeline object.
 */

#pragma once

#include "main.h"

namespace vku {
    namespace gfx {

        class Framebuffer;
        class Shader;

        class GraphicsPipeline final
        {
        public:
            GraphicsPipeline(const LogicalDevice* device, const std::vector<std::shared_ptr<Shader>>& shaders, const glm::uvec2& size, unsigned int numBlendAttachments);
            GraphicsPipeline(const GraphicsPipeline&);
            GraphicsPipeline& operator=(const GraphicsPipeline&);
            GraphicsPipeline(GraphicsPipeline&&) noexcept;
            GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept;
            ~GraphicsPipeline();

            void ResetShaders(const std::vector<std::shared_ptr<Shader>>& shaders);
            template<class Vertex> void ResetVertexInput() const;
            void ResetFramebuffer(const glm::uvec2& size, unsigned int numViewports, unsigned int numScissors) const;
            void CreatePipeline(bool keepState, vk::RenderPass renderPass, unsigned int subpass, vk::PipelineLayout pipelineLayout);
            vk::Pipeline GetPipeline() const { return vkPipeline_; }

            vk::Viewport& GetViewport(unsigned int idx) const { assert(state_); return state_->viewports_[idx]; }
            vk::Rect2D& GetScissor(unsigned int idx) const { assert(state_); return state_->scissors_[idx]; }
            vk::PipelineMultisampleStateCreateInfo& GetMultisampling() const { assert(state_); return state_->multisampling_; }
            vk::PipelineRasterizationStateCreateInfo& GetRasterizer() const { assert(state_); return state_->rasterizer_; }
            vk::PipelineDepthStencilStateCreateInfo& GetDepthStencil() const { assert(state_); return state_->depthStencil_; }
            vk::PipelineTessellationStateCreateInfo& GetTesselation() const { assert(state_); return state_->tesselation_; }
            vk::PipelineColorBlendAttachmentState& GetColorBlendAttachment(unsigned int idx) const { assert(state_); return state_->colorBlendAttachments_[idx]; }
            vk::PipelineColorBlendStateCreateInfo& GetColorBlending() const { assert(state_); return state_->colorBlending_; }
            std::vector<vk::DynamicState>& GetDynamicStates() const { assert(state_); return state_->dynamicStates_; }

        private:

            struct State final
            {
                /** Holds the information about the shaders used. */
                std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos_;
                /** Holds the vertex input state. */
                vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo_;
                /** Holds the input assembly state. */
                vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo_;
                /** Holds the viewports. */
                std::vector<vk::Viewport> viewports_;
                /** Holds the scissors. */
                std::vector<vk::Rect2D> scissors_;
                /** Holds the viewport state */
                vk::PipelineViewportStateCreateInfo viewportState_;
                /** Holds the multisampling state. */
                vk::PipelineMultisampleStateCreateInfo multisampling_;
                /** Holds the depth stencil state. */
                vk::PipelineDepthStencilStateCreateInfo depthStencil_;
                /** Holds the rasterizer state. */
                vk::PipelineRasterizationStateCreateInfo rasterizer_;
                /** Holds the tesselation state. */
                vk::PipelineTessellationStateCreateInfo tesselation_;
                /** Holds the color blend attachments. */
                std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments_;
                /** Holds the blend state. */
                vk::PipelineColorBlendStateCreateInfo colorBlending_;
                /** Holds the dynamic states. */
                std::vector<vk::DynamicState> dynamicStates_;
            };

            /** Holds the device. */
            const LogicalDevice* device_;
            /** Holds the shaders used in this pipeline. */
            std::vector<std::shared_ptr<Shader>> shaders_;
            /** Holds the state. */
            std::unique_ptr<State> state_;
            /** Holds the pipeline. */
            vk::Pipeline vkPipeline_;
        };

        template <class Vertex>
        void GraphicsPipeline::ResetVertexInput() const
        {
            state_->vertexInputCreateInfo_ = vk::PipelineVertexInputStateCreateInfo{ vk::PipelineVertexInputStateCreateFlags(), 1, &Vertex::bindingDescription_,
                static_cast<uint32_t>(Vertex::attributeDescriptions_.size()), Vertex::attributeDescriptions_.data() };
        }
    }
}

