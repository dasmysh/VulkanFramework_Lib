/**
 * @file   RenderElement.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.28
 *
 * @brief  A single element for a renderer to process.
 */

#pragma once

#include <tuple>
#include <glm/mat4x4.hpp>

#include "main.h"
#include "core/math/primitives.h"
#include "gfx/vk/wrappers/PipelineLayout.h"
#include "gfx/vk/pipeline/GraphicsPipeline.h"

namespace vkfw_core::gfx {

    class DescriptorSet;
    class DeviceBuffer;
    class PipelineLayout;
    class UniformBufferObject;
    class GraphicsPipeline;

    class RenderElement final
    {
    public:
        using BufferReference = std::pair<const DeviceBuffer*, vk::DeviceSize>;
        using UBOBinding = std::tuple<const DescriptorSet*, std::uint32_t, std::uint32_t>;
        using DescSetBinding = std::pair<const DescriptorSet*, std::uint32_t>;

        inline RenderElement(bool isTransparent, const GraphicsPipeline& pipeline, const PipelineLayout& pipelineLayout);
        inline RenderElement(bool isTransparent, const RenderElement& referenceElement);

        inline RenderElement& BindVertexBuffer(BufferReference vtxBuffer);
        inline RenderElement& BindIndexBuffer(BufferReference idxBuffer);
        inline RenderElement& BindCameraMatricesUBO(UBOBinding cameraMatricesUBO);
        inline RenderElement& BindWorldMatricesUBO(UBOBinding worldMatricesUBO);
        inline RenderElement& BindUBO(UBOBinding ubo);
        inline RenderElement& BindDescriptorSet(DescSetBinding descSet);
        inline RenderElement& DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
            const math::AABB3<float>& boundingBox);

        inline const RenderElement& DrawElement(const CommandBuffer& cmdBuffer, const RenderElement* lastElement = nullptr) const;

        friend bool operator<(const RenderElement& l, const RenderElement& r)
        {
            if (l.m_isTransparent && !r.m_isTransparent) {
                return false;
            }
            if (r.m_isTransparent && !l.m_isTransparent) {
                return true;
            }
            if (l.m_isTransparent && r.m_isTransparent) {
                return l.m_cameraDistance < r.m_cameraDistance;
            }

            if (l.m_pipeline < r.m_pipeline) { return true; }
            if (l.m_vertexBuffer.first < r.m_vertexBuffer.first) { return true; }
            if (l.m_indexBuffer.first < r.m_indexBuffer.first) { return true; }
            return l.m_cameraDistance > r.m_cameraDistance;
        }

    private:
        bool m_isTransparent;
        const GraphicsPipeline* m_pipeline;
        const PipelineLayout* m_pipelineLayout;

        BufferReference m_vertexBuffer = BufferReference(nullptr, 0);
        BufferReference m_indexBuffer = BufferReference(nullptr, 0);
        UBOBinding m_cameraMatricesUBO = UBOBinding(nullptr, 0, 0);
        UBOBinding m_worldMatricesUBO = UBOBinding(nullptr, 0, 0);
        std::vector<UBOBinding> m_generalUBOs;
        std::vector<DescSetBinding> m_generalDescSets;

        std::uint32_t m_indexCount = 0;
        std::uint32_t m_instanceCount = 0;
        std::uint32_t m_firstIndex = 0;
        std::uint32_t m_vertexOffset = 0;
        std::uint32_t m_firstInstance = 0;
        float m_cameraDistance = 0.0f;

    };

    RenderElement::RenderElement(bool isTransparent, const GraphicsPipeline& pipeline, const PipelineLayout& pipelineLayout) :
        m_isTransparent{ isTransparent },
        m_pipeline{ &pipeline },
        m_pipelineLayout{ &pipelineLayout }
    {
    }

    RenderElement::RenderElement(bool isTransparent, const RenderElement& referenceElement) :
        m_isTransparent{ isTransparent },
        m_pipeline{ referenceElement.m_pipeline },
        m_pipelineLayout{ referenceElement.m_pipelineLayout },
        m_vertexBuffer{ referenceElement.m_vertexBuffer },
        m_indexBuffer{ referenceElement.m_indexBuffer },
        m_cameraMatricesUBO{ referenceElement.m_cameraMatricesUBO },
        m_worldMatricesUBO{ referenceElement.m_worldMatricesUBO }
    {
    }

    RenderElement& RenderElement::BindVertexBuffer(BufferReference vtxBuffer)
    {
        m_vertexBuffer = vtxBuffer;
        return *this;
    }

    RenderElement& RenderElement::BindIndexBuffer(BufferReference idxBuffer)
    {
        m_indexBuffer = idxBuffer;
        return *this;
    }

    RenderElement& RenderElement::BindCameraMatricesUBO(UBOBinding cameraMatricesUBO)
    {
        m_cameraMatricesUBO = std::move(cameraMatricesUBO);
        return *this;
    }

    RenderElement& RenderElement::BindWorldMatricesUBO(UBOBinding worldMatricesUBO)
    {
        m_worldMatricesUBO = std::move(worldMatricesUBO);
        return *this;
    }

    RenderElement& RenderElement::BindUBO(UBOBinding ubo)
    {
        m_generalUBOs.emplace_back(std::move(ubo));
        return *this;
    }

    RenderElement& RenderElement::BindDescriptorSet(DescSetBinding descSet)
    {
        m_generalDescSets.emplace_back(std::move(descSet));
        return *this;
    }

    RenderElement& RenderElement::DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
        std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
        const math::AABB3<float>& boundingBox)
    {
        m_indexCount = indexCount;
        m_instanceCount = instanceCount;
        m_firstIndex = firstIndex;
        m_vertexOffset = vertexOffset;
        m_firstInstance = firstInstance;

        auto bbCenter = 0.5f * (boundingBox.m_minmax[0] + boundingBox.m_minmax[1]);
        auto viewAxis = glm::transpose(glm::mat3(viewMatrix))[2];
        m_cameraDistance = glm::dot(viewAxis, bbCenter);
        return *this;
    }

    const RenderElement& RenderElement::DrawElement(const CommandBuffer& cmdBuffer, const RenderElement* lastElement /*= nullptr*/) const
    {
        if ((lastElement == nullptr) || lastElement->m_pipeline == m_pipeline) {
            cmdBuffer.GetHandle().bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->GetHandle());
        }
        if ((lastElement == nullptr) || lastElement->m_vertexBuffer == m_vertexBuffer) {
            cmdBuffer.GetHandle().bindVertexBuffers(0, 1, m_vertexBuffer.first->GetHandlePtr(), &m_vertexBuffer.second);
        }
        if ((lastElement == nullptr) || lastElement->m_indexBuffer == m_indexBuffer) {
            cmdBuffer.GetHandle().bindIndexBuffer(m_indexBuffer.first->GetHandle(), m_indexBuffer.second,
                                                  vk::IndexType::eUint32);
        }

        cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout->GetHandle(),
                                     std::get<1>(m_cameraMatricesUBO),
                                     std::get<0>(m_cameraMatricesUBO)->GetHandle(), std::get<2>(m_cameraMatricesUBO));

        cmdBuffer.GetHandle().bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, m_pipelineLayout->GetHandle(), std::get<1>(m_worldMatricesUBO),
            std::get<0>(m_worldMatricesUBO)->GetHandle(), std::get<2>(m_worldMatricesUBO));

        for (const auto& ubo : m_generalUBOs) {
            cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout->GetHandle(),
                                                     std::get<1>(ubo), std::get<0>(ubo)->GetHandle(), std::get<2>(ubo));
        }

        for (const auto& ds : m_generalDescSets) {
            cmdBuffer.GetHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout->GetHandle(),
                                                     ds.second, ds.first->GetHandle(), nullptr);
        }

        cmdBuffer.GetHandle().drawIndexed(m_indexCount, m_instanceCount, m_firstIndex, m_vertexOffset, m_firstInstance);
        return *this;
    }

}
