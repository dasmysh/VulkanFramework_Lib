/**
 * @file   FullscreenQuad.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.11.04
 *
 * @brief  Implementation of the fullscreen quad class.
 */

#include "gfx/renderer/FullscreenQuad.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/pipeline/GraphicsPipeline.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx {

    FullscreenQuad::FullscreenQuad(const std::string& fragmentShader, unsigned int numBlendAttachments)
        : m_shaderFiles{"shader/fullscreen.vert", fragmentShader}
        , m_numBlendAttachments{numBlendAttachments}
    {
    }

    FullscreenQuad::~FullscreenQuad() = default;

    void FullscreenQuad::CreatePipeline(LogicalDevice* device, const glm::uvec2& screenSize,
                                        const RenderPass& renderPass, unsigned int subpass,
                                        const PipelineLayout& pipelineLayout)
    {
        m_pipeline = device->CreateGraphicsPipeline(m_shaderFiles, screenSize, m_numBlendAttachments);
        m_pipeline->ResetVertexInput();
        m_pipeline->CreatePipeline(true, renderPass, subpass, pipelineLayout);
    }

    void FullscreenQuad::Render(CommandBuffer& cmdBuffer)
    {
        cmdBuffer.GetHandle().bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->GetHandle());
        // bind descriptor sets??

        cmdBuffer.GetHandle().draw(3, 1, 0, 0);
    }
}
