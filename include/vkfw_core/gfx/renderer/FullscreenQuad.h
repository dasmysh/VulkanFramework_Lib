/**
 * @file   FullscreenQuad.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.11.04
 *
 * @brief  Helper class to render full screen.
 */

#pragma once

#include "main.h"
#include <glm/fwd.hpp>

namespace vkfw_core::gfx {

    class CommandBuffer;
    class GraphicsPipeline;
    class LogicalDevice;
    class RenderPass;
    class PipelineLayout;

    class FullscreenQuad
    {
    public:
        FullscreenQuad(const std::string& fragmentShader, unsigned int numBlendAttachments);
        ~FullscreenQuad();

        void CreatePipeline(LogicalDevice* device, const glm::uvec2& screenSize, const RenderPass& renderPass,
                            unsigned int subpass, const PipelineLayout& pipelineLayout);
        void Render(CommandBuffer& cmdBuffer);

    private:
        /** Holds the shader files to create the pipeline from. */
        std::vector<std::string> m_shaderFiles;
        /** Holds the number of blend attachments. */
        unsigned int m_numBlendAttachments;
        /** Holds the pipeline. */
        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
