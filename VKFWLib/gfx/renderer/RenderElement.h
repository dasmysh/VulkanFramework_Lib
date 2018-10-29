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

namespace vku::gfx {

    class DeviceBuffer;
    class UniformBufferObject;

    class RenderElement final
    {
    public:
        using BufferReference = std::pair<const DeviceBuffer*, std::size_t>;
        using UBOBinding = std::tuple<const UniformBufferObject*, std::uint32_t, std::size_t>;
        using DescSetBinding = std::pair<vk::DescriptorSet, std::uint32_t>;

        RenderElement(bool isTransparent, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout);
        RenderElement(bool isTransparent, const RenderElement& referenceElement);

        inline void BindVertexBuffer(BufferReference vtxBuffer);
        inline void BindIndexBuffer(BufferReference idxBuffer);
        inline void BindWorldMatricesUBO(UBOBinding worldMatricesUBO);
        inline void BindUBO(UBOBinding ubo);
        inline void BindDescriptorSet(DescSetBinding descSet);
        inline void DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const math::AABB3<float>& boundingBox);

        inline void DrawElement(vk::CommandBuffer cmdBuffer, const RenderElement* lastElement = nullptr);

        friend bool operator<(const RenderElement& l, const RenderElement& r)
        {
            if (l.isTransparent_ && !r.isTransparent_) return false;
            else if (r.isTransparent_ && !l.isTransparent_) return true;
            else if (l.isTransparent_ && r.isTransparent_) return l.cameraDistance_ < r.cameraDistance_;

            if (l.pipeline_ < r.pipeline_) return true;
            if (l.vertexBuffer_.first < r.vertexBuffer_.first) return true;
            if (l.indexBuffer_.first < r.indexBuffer_.first) return true;
            return l.cameraDistance_ > r.cameraDistance_;
        }

    private:
        bool isTransparent_;
        vk::Pipeline pipeline_;
        vk::PipelineLayout pipelineLayout_;

        BufferReference vertexBuffer_ = BufferReference(nullptr, 0);
        BufferReference indexBuffer_ = BufferReference(nullptr, 0);
        UBOBinding worldMatricesUBO_ = UBOBinding(nullptr, 0, 0);
        std::vector<UBOBinding> generalUBOs_;
        std::vector<DescSetBinding> generalDescSets_;

        std::uint32_t indexCount_ = 0;
        std::uint32_t instanceCount_ = 0;
        std::uint32_t firstIndex_ = 0;
        std::uint32_t vertexOffset_ = 0;
        std::uint32_t firstInstance_ = 0;
        float cameraDistance_ = 0.0f;

    };

    RenderElement::RenderElement(bool isTransparent, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout) :
        isTransparent_{ isTransparent },
        pipeline_{ pipeline },
        pipelineLayout_{ pipelineLayout }
    {
    }

    RenderElement::RenderElement(bool isTransparent, const RenderElement& referenceElement) :
        isTransparent_{ isTransparent },
        pipeline_{ referenceElement.pipeline_ },
        pipelineLayout_{ referenceElement.pipelineLayout_ },
        vertexBuffer_{ referenceElement.vertexBuffer_ },
        indexBuffer_{ referenceElement.indexBuffer_ }
    {
    }

    void RenderElement::BindVertexBuffer(BufferReference vtxBuffer)
    {
        vertexBuffer_ = vtxBuffer;
    }

    void RenderElement::BindIndexBuffer(BufferReference idxBuffer)
    {
        indexBuffer_ = idxBuffer;
    }

    void RenderElement::BindWorldMatricesUBO(UBOBinding worldMatricesUBO)
    {
        worldMatricesUBO_ = worldMatricesUBO;
    }

    void RenderElement::BindUBO(UBOBinding ubo)
    {
        generalUBOs_.push_back(ubo);
    }

    void RenderElement::BindDescriptorSet(DescSetBinding descSet)
    {
        generalDescSets_.push_back(descSet);
    }

    void RenderElement::DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
        std::uint32_t vertexOffset, std::uint32_t firstInstance, const math::AABB3<float>& boundingBox)
    {
        indexCount_ = indexCount;
        instanceCount_ = instanceCount;
        firstIndex_ = firstIndex;
        vertexOffset_ = vertexOffset;
        firstInstance_ = firstInstance;

        // TODO: add camera distance [10/29/2018 Sebastian Maisch]
    }

}
