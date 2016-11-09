/**
 * @file   GraphicsPipeline.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Implementation of a vulkan graphics pipeline object.
 */

#include "GraphicsPipeline.h"
#include "LogicalDevice.h"
#include "core/resources/ShaderManager.h"
#include "Framebuffer.h"

namespace vku { namespace gfx {

    GraphicsPipeline::GraphicsPipeline(gfx::LogicalDevice* device, const std::vector<std::shared_ptr<Shader>>& shaders, const glm::uvec2& size, unsigned int numBlendAttachments) :
        device_{ device },
        shaders_{ shaders },
        state_{ std::make_unique<State>() }
    {
        ResetShaders(shaders);

        // TODO: gets filled by model later (vertexInputCreateInfo_, inputAssemblyCreateInfo_). [10/31/2016 Sebastian Maisch]
        state_->inputAssemblyCreateInfo_.setTopology(vk::PrimitiveTopology::eTriangleList);

        ResetFramebuffer(size, 1, 1);

        state_->rasterizer_ = vk::PipelineRasterizationStateCreateInfo{ vk::PipelineRasterizationStateCreateFlags(), VK_FALSE,
            VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f };
        vk::StencilOpState frontStencilOpState{};
        vk::StencilOpState backStencilOpState{};
        state_->depthStencil_ = vk::PipelineDepthStencilStateCreateInfo{ vk::PipelineDepthStencilStateCreateFlags(),
            VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE,
            VK_FALSE, frontStencilOpState, backStencilOpState, 0.0f, 0.0f };

        state_->colorBlendAttachments_.resize(numBlendAttachments);
        for (auto& blendAttachment : state_->colorBlendAttachments_) {
            blendAttachment = vk::PipelineColorBlendAttachmentState{ VK_FALSE,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        }

        state_->colorBlending_ = vk::PipelineColorBlendStateCreateInfo{ vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eCopy,
            static_cast<uint32_t>(state_->colorBlendAttachments_.size()), state_->colorBlendAttachments_.data(), {{ 0.0f, 0.0f, 0.0f, 0.0f }} };

        // state_->pipelineLayoutInfo_ = vk::PipelineLayoutCreateInfo{ vk::PipelineLayoutCreateFlags(), };
        /*pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = 0; // Optional


        vk::GraphicsPipelineCreateInfo gpCreateInfo{ , };
        device_->GetDevice().createGraphicsPipeline(vk::PipelineCache(), );*/
    }


    GraphicsPipeline::~GraphicsPipeline()
    {
        if (vkPipeline_) device_->GetDevice().destroyPipeline(vkPipeline_);
        vkPipeline_ = vk::Pipeline();
    }

    void GraphicsPipeline::ResetShaders(const std::vector<std::shared_ptr<Shader>>& shaders)
    {
        assert(state_);
        shaders_ = shaders;
        state_->shaderStageInfos_.resize(shaders.size());
        for (auto i = 0U; i < shaders.size(); ++i) {
            shaders[i]->FillShaderStageInfo(state_->shaderStageInfos_[i]);
        }
    }

    void GraphicsPipeline::ResetFramebuffer(const glm::uvec2& size, unsigned int numViewports, unsigned int numScissors) const
    {
        state_->viewports_.resize(numViewports);
        for (auto& viewport : state_->viewports_) viewport = vk::Viewport{ 0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.0f, 1.0f };
        state_->scissors_.resize(numScissors);
        for (auto& scissor : state_->scissors_) scissor = vk::Rect2D{ vk::Offset2D(), vk::Extent2D{ size.x, size.y } };
        
        state_->viewportState_ = vk::PipelineViewportStateCreateInfo{ vk::PipelineViewportStateCreateFlags(), numViewports, state_->viewports_.data(), numScissors, state_->scissors_.data() };
        state_->multisampling_ = vk::PipelineMultisampleStateCreateInfo{ vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE };
    }

    void GraphicsPipeline::CreatePipeline(bool keepState, vk::RenderPass renderPass, unsigned int subpass, vk::PipelineLayout pipelineLayout)
    {
        assert(state_);
        vk::PipelineDynamicStateCreateInfo dynamicState{ vk::PipelineDynamicStateCreateFlags(), static_cast<uint32_t>(state_->dynamicStates_.size()), state_->dynamicStates_.data() };

        // auto pipelineLayout = device_->GetDevice().createPipelineLayout(state_->pipelineLayoutInfo_);
        // device_->GetDevice().destroyP
        
        vk::GraphicsPipelineCreateInfo pipelineInfo{ vk::PipelineCreateFlags(),
            static_cast<uint32_t>(state_->shaderStageInfos_.size()), state_->shaderStageInfos_.data(),
            &state_->vertexInputCreateInfo_, &state_->inputAssemblyCreateInfo_, &state_->tesselation_,
            &state_->viewportState_, &state_->rasterizer_, &state_->multisampling_, &state_->depthStencil_,
            &state_->colorBlending_, &dynamicState, pipelineLayout, renderPass, subpass };

        vkPipeline_ = device_->GetDevice().createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

        if (!keepState) state_.reset();
    }
}}
