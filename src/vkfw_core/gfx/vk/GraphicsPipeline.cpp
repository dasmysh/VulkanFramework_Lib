/**
 * @file   GraphicsPipeline.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Implementation of a vulkan graphics pipeline object.
 */

#include "gfx/vk/GraphicsPipeline.h"
#include "gfx/vk/LogicalDevice.h"
#include "core/resources/ShaderManager.h"

namespace vkfw_core::gfx {

    GraphicsPipeline::GraphicsPipeline(const LogicalDevice* device, const std::vector<std::shared_ptr<Shader>>& shaders, const glm::uvec2& size, unsigned int numBlendAttachments) :
        m_device{ device },
        m_shaders{ shaders },
        m_state{ std::make_unique<State>() }
    {
        ResetShaders(shaders);

        m_state->m_inputAssemblyCreateInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
        m_state->m_tesselation.setPatchControlPoints(1);

        ResetFramebuffer(size, 1, 1);

        m_state->m_rasterizer = vk::PipelineRasterizationStateCreateInfo{ vk::PipelineRasterizationStateCreateFlags(), VK_FALSE,
            VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f };
        vk::StencilOpState frontStencilOpState{};
        vk::StencilOpState backStencilOpState{};
        m_state->m_depthStencil = vk::PipelineDepthStencilStateCreateInfo{ vk::PipelineDepthStencilStateCreateFlags(),
            VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE,
            VK_FALSE, frontStencilOpState, backStencilOpState, 0.0f, 1.0f };

        m_state->m_colorBlendAttachments.resize(numBlendAttachments);
        for (auto& blendAttachment : m_state->m_colorBlendAttachments) {
            blendAttachment = vk::PipelineColorBlendAttachmentState{ VK_FALSE,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        }

        m_state->m_colorBlending = vk::PipelineColorBlendStateCreateInfo{ vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eCopy,
                                                  static_cast<std::uint32_t>(m_state->m_colorBlendAttachments.size()),
                                                  m_state->m_colorBlendAttachments.data(),
                                                  {{0.0f, 0.0f, 0.0f, 0.0f}}};

        m_state->m_dynamicStates.push_back(vk::DynamicState::eLineWidth);

        // state_->pipelineLayoutInfo_ = vk::PipelineLayoutCreateInfo{ vk::PipelineLayoutCreateFlags(), };
        /*pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = 0; // Optional*/
    }

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& rhs) noexcept :
    m_device{ rhs.m_device },
        m_shaders{ std::move(rhs.m_shaders) },
        m_state{ std::move(rhs.m_state) },
        m_vkPipeline{ std::move(rhs.m_vkPipeline) }
    {
    }

    GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~GraphicsPipeline();
            m_device = rhs.m_device;
            m_shaders = std::move(rhs.m_shaders);
            m_state = std::move(rhs.m_state);
            m_vkPipeline = std::move(rhs.m_vkPipeline);
        }
        return *this;
    }

    GraphicsPipeline::~GraphicsPipeline() = default;

    void GraphicsPipeline::ResetShaders(const std::vector<std::shared_ptr<Shader>>& shaders)
    {
        assert(m_state);
        m_shaders = shaders;
        m_state->m_shaderStageInfos.resize(shaders.size());
        for (auto i = 0U; i < shaders.size(); ++i) { shaders[i]->FillShaderStageInfo(m_state->m_shaderStageInfos[i]); }
    }

    void GraphicsPipeline::ResetFramebuffer(const glm::uvec2& size, unsigned int numViewports, unsigned int numScissors) const
    {
        m_state->m_viewports.resize(numViewports);
        for (auto& viewport : m_state->m_viewports) {
            viewport = vk::Viewport{0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.0f, 1.0f};
        }
        m_state->m_scissors.resize(numScissors);
        for (auto& scissor : m_state->m_scissors) { scissor = vk::Rect2D{vk::Offset2D(), vk::Extent2D{size.x, size.y}}; }

        m_state->m_viewportState = vk::PipelineViewportStateCreateInfo{ vk::PipelineViewportStateCreateFlags(), numViewports, m_state->m_viewports.data(), numScissors, m_state->m_scissors.data() };
        m_state->m_multisampling = vk::PipelineMultisampleStateCreateInfo{ vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE };
    }

    void GraphicsPipeline::CreatePipeline(bool keepState, vk::RenderPass renderPass, unsigned int subpass, vk::PipelineLayout pipelineLayout)
    {
        assert(m_state);
        vk::PipelineDynamicStateCreateInfo dynamicState{vk::PipelineDynamicStateCreateFlags(),
                                                        static_cast<std::uint32_t>(m_state->m_dynamicStates.size()),
                                                        m_state->m_dynamicStates.data()};

        // TODO: allow derivates? [10/30/2018 Sebastian Maisch]
        vk::GraphicsPipelineCreateInfo pipelineInfo{ vk::PipelineCreateFlags(),
                                                    static_cast<std::uint32_t>(m_state->m_shaderStageInfos.size()),
                                                    m_state->m_shaderStageInfos.data(),
            &m_state->m_vertexInputCreateInfo, &m_state->m_inputAssemblyCreateInfo, &m_state->m_tesselation,
            &m_state->m_viewportState, &m_state->m_rasterizer, &m_state->m_multisampling, &m_state->m_depthStencil,
            &m_state->m_colorBlending, &dynamicState, pipelineLayout, renderPass, subpass };

        m_vkPipeline = m_device->GetDevice().createGraphicsPipelineUnique(vk::PipelineCache(), pipelineInfo);

        if (!keepState) { m_state.reset(); }
    }
}
