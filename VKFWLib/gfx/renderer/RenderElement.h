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

        inline void BindVertexBuffer(BufferReference vtxBuffer);
        inline void BindIndexBuffer(BufferReference idxBuffer);
        inline void BindWorldMatricesUBO(UBOBinding worldMatricesUBO);
        inline void BindUBO(UBOBinding ubo);
        inline void BindDescriptorSet(DescSetBinding descSet);
        inline void DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const AABB3& boundingBox);

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

        BufferReference vertexBuffer_;
        BufferReference indexBuffer_;
        UBOBinding worldMatricesUBO_;
        std::vector<UBOBinding> generalUBOs_;
        std::vector<DescSetBinding> generalDescSets_;

        std::uint32_t indexCount_;
        std::uint32_t instanceCount_;
        std::uint32_t firstIndex_;
        std::uint32_t vertexOffset_;
        std::uint32_t firstInstance_;
        float cameraDistance_;

    };

    RenderElement::RenderElement(bool isTransparent, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout)
    {
        // TODO: implmenent [10/29/2018 Sebastian Maisch]
    }

    void RenderElement::BindVertexBuffer(const BufferReference& vtxBuffer)
    {
        vertexBuffer_ = vtxBuffer;
    }

    void RenderElement::BindIndexBuffer(const BufferReference& idxBuffer)
    {
        indexBuffer_ = idxBuffer;
    }

    void RenderElement::BindWorldMatricesUBO(const UBOBinding& worldMatricesUBO)
    {
        worldMatricesUBO_ = worldMatricesUBO;
    }

    void RenderElement::BindUBO(const UBOBinding& ubo)
    {
        generalUBOs_.push_back(ubo);
    }

    void RenderElement::BindDescriptorSet(DescSetBinding descSet)
    {
        generalDescSets_.push_back(descSet);
    }

    void RenderElement::DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
        std::uint32_t vertexOffset, std::uint32_t firstInstance, const AABB3& boundingBox)
    {
        indexCount_ = indexCount;
        instanceCount_ = instanceCount;
        firstIndex_ = firstIndex;
        vertexOffset_ = vertexOffset;
        firstInstance_ = firstInstance;

        // TODO: add camera distance [10/29/2018 Sebastian Maisch]
    }

}
