/**
 * @file   RenderList.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.10.30
 *
 * @brief  Implementation of a list of RenderElement objects.
 */

#pragma once

#include "gfx/renderer/RenderElement.h"
#include "gfx/camera/CameraBase.h"

namespace vkfw_core::gfx {

    class RenderList final
    {
    public:
        using BufferReference = RenderElement::BufferReference;
        using DescSetBinding = RenderElement::DescSetBinding;
        using UBOBinding = RenderElement::UBOBinding;

        inline RenderList(const CameraBase* camera, const UBOBinding& cameraUBO);

        inline void SetCurrentPipeline(const PipelineLayout& currentPipelineLayout,
                                       const GraphicsPipeline& currentOpaquePipeline,
                                       const GraphicsPipeline& currentTransparentPipeline);
        inline void SetCurrentGeometry(BufferReference currentVertexBuffer, BufferReference currentIndexBuffer);
        inline void SetCurrentWorldMatrices(const UBOBinding& currentWorldMatrices);

        inline RenderElement& AddOpaqueElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
            const math::AABB3<float>& boundingBox);
        inline RenderElement& AddTransparentElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
            const math::AABB3<float>& boundingBox);

        inline void Render(const LogicalDevice* device, const CommandBuffer& cmdBuffer);

    private:
        std::vector<RenderElement> m_opaqueElements;
        std::vector<RenderElement> m_transparentElements;

        const CameraBase* m_camera; // NOLINT(clang-diagnostic-unused-private-field)
        UBOBinding m_cameraMatricesUBO;
        const PipelineLayout* m_currentPipelineLayout = nullptr;
        const GraphicsPipeline* m_currentOpaquePipeline = nullptr;
        const GraphicsPipeline* m_currentTransparentPipeline = nullptr;

        BufferReference m_currentVertexBuffer = BufferReference(nullptr, 0);
        BufferReference m_currentIndexBuffer = BufferReference(nullptr, 0);

        UBOBinding m_currentWorldMatrices = UBOBinding(nullptr, 0, 0);
    };

    RenderList::RenderList(const CameraBase* camera, const UBOBinding& cameraUBO)
        :
        m_camera{ camera },
        m_cameraMatricesUBO{ cameraUBO }
    {
    }

    void RenderList::SetCurrentPipeline(const PipelineLayout& currentPipelineLayout,
                                        const GraphicsPipeline& currentOpaquePipeline,
                                        const GraphicsPipeline& currentTransparentPipeline)
    {
        m_currentPipelineLayout = &currentPipelineLayout;
        m_currentOpaquePipeline = &currentOpaquePipeline;
        m_currentTransparentPipeline = &currentTransparentPipeline;
    }

    void RenderList::SetCurrentGeometry(BufferReference currentVertexBuffer, BufferReference currentIndexBuffer)
    {
        m_currentVertexBuffer = currentVertexBuffer;
        m_currentIndexBuffer = currentIndexBuffer;
    }

    void RenderList::SetCurrentWorldMatrices(const UBOBinding& currentWorldMatrices)
    {
        m_currentWorldMatrices = currentWorldMatrices;
    }

    vkfw_core::gfx::RenderElement& RenderList::AddOpaqueElement(std::uint32_t indexCount, std::uint32_t instanceCount,
        std::uint32_t firstIndex, std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
        const math::AABB3<float>& boundingBox)
    {
        auto& result = m_opaqueElements.emplace_back(false, *m_currentOpaquePipeline, *m_currentPipelineLayout);
        result.BindVertexBuffer(m_currentVertexBuffer);
        result.BindIndexBuffer(m_currentIndexBuffer);
        result.BindCameraMatricesUBO(m_cameraMatricesUBO);
        result.BindWorldMatricesUBO(m_currentWorldMatrices);
        result.DrawGeometry(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, viewMatrix, boundingBox);
        return result;
    }

    vkfw_core::gfx::RenderElement& RenderList::AddTransparentElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix, const math::AABB3<float>& boundingBox)
    {
        auto& result = m_transparentElements.emplace_back(true, *m_currentTransparentPipeline, *m_currentPipelineLayout);
        result.BindVertexBuffer(m_currentVertexBuffer);
        result.BindIndexBuffer(m_currentIndexBuffer);
        result.BindCameraMatricesUBO(m_cameraMatricesUBO);
        result.BindWorldMatricesUBO(m_currentWorldMatrices);
        result.DrawGeometry(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, viewMatrix, boundingBox);
        return result;
    }

    void RenderList::Render(const LogicalDevice* device, const CommandBuffer& cmdBuffer)
    {
        std::sort(m_opaqueElements.begin(), m_opaqueElements.end());
        std::sort(m_transparentElements.begin(), m_transparentElements.end());

        const RenderElement* lastElement = nullptr;
        for (const auto& re : m_opaqueElements) { lastElement = &re.DrawElement(device, cmdBuffer, lastElement); }

        lastElement = nullptr;
        for (const auto& re : m_transparentElements) { lastElement = &re.DrawElement(device, cmdBuffer, lastElement); }
    }

}
