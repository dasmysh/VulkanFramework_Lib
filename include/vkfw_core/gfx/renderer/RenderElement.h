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

namespace vkfw_core::gfx {

    class DeviceBuffer;
    class UniformBufferObject;

    class RenderElement final
    {
    public:
        using BufferReference = std::pair<const DeviceBuffer*, vk::DeviceSize>;
        using UBOBinding = std::tuple<const UniformBufferObject*, std::uint32_t, std::size_t>;
        using DescSetBinding = std::pair<vk::DescriptorSet, std::uint32_t>;

        inline RenderElement(bool isTransparent, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout);
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

        inline const RenderElement& DrawElement(vk::CommandBuffer cmdBuffer, const RenderElement* lastElement = nullptr) const;

        friend bool operator<(const RenderElement& l, const RenderElement& r)
        {
            if (l.isTransparent_ && !r.isTransparent_) {
                return false;
            }
            if (r.isTransparent_ && !l.isTransparent_) {
                return true;
            }
            if (l.isTransparent_ && r.isTransparent_) {
                return l.cameraDistance_ < r.cameraDistance_;
            }

            if (l.pipeline_ < r.pipeline_) { return true; }
            if (l.vertexBuffer_.first < r.vertexBuffer_.first) { return true; }
            if (l.indexBuffer_.first < r.indexBuffer_.first) { return true; }
            return l.cameraDistance_ > r.cameraDistance_;
        }

    private:
        bool isTransparent_;
        vk::Pipeline pipeline_;
        vk::PipelineLayout pipelineLayout_;

        BufferReference vertexBuffer_ = BufferReference(nullptr, 0);
        BufferReference indexBuffer_ = BufferReference(nullptr, 0);
        UBOBinding cameraMatricesUBO_ = UBOBinding(nullptr, 0, 0);
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
        indexBuffer_{ referenceElement.indexBuffer_ },
        cameraMatricesUBO_{ referenceElement.cameraMatricesUBO_ },
        worldMatricesUBO_{ referenceElement.worldMatricesUBO_ }
    {
    }

    RenderElement& RenderElement::BindVertexBuffer(BufferReference vtxBuffer)
    {
        vertexBuffer_ = vtxBuffer;
        return *this;
    }

    RenderElement& RenderElement::BindIndexBuffer(BufferReference idxBuffer)
    {
        indexBuffer_ = idxBuffer;
        return *this;
    }

    RenderElement& RenderElement::BindCameraMatricesUBO(UBOBinding cameraMatricesUBO)
    {
        cameraMatricesUBO_ = std::move(cameraMatricesUBO);
        return *this;
    }

    RenderElement& RenderElement::BindWorldMatricesUBO(UBOBinding worldMatricesUBO)
    {
        worldMatricesUBO_ = std::move(worldMatricesUBO);
        return *this;
    }

    RenderElement& RenderElement::BindUBO(UBOBinding ubo)
    {
        generalUBOs_.emplace_back(std::move(ubo));
        return *this;
    }

    RenderElement& RenderElement::BindDescriptorSet(DescSetBinding descSet)
    {
        generalDescSets_.emplace_back(std::move(descSet));
        return *this;
    }

    RenderElement& RenderElement::DrawGeometry(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
        std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
        const math::AABB3<float>& boundingBox)
    {
        indexCount_ = indexCount;
        instanceCount_ = instanceCount;
        firstIndex_ = firstIndex;
        vertexOffset_ = vertexOffset;
        firstInstance_ = firstInstance;

        auto bbCenter = 0.5f * (boundingBox.minmax_[0] + boundingBox.minmax_[1]);
        auto viewAxis = glm::transpose(glm::mat3(viewMatrix))[2];
        cameraDistance_ = glm::dot(viewAxis, bbCenter);
        return *this;
    }

    const RenderElement& RenderElement::DrawElement(vk::CommandBuffer cmdBuffer, const RenderElement* lastElement /*= nullptr*/) const
    {
        if ((lastElement == nullptr) || lastElement->pipeline_ == pipeline_) {
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
        }
        if ((lastElement == nullptr) || lastElement->vertexBuffer_ == vertexBuffer_) {
            cmdBuffer.bindVertexBuffers(0, 1, vertexBuffer_.first->GetBufferPtr(), &vertexBuffer_.second);
        }
        if ((lastElement == nullptr) || lastElement->indexBuffer_ == indexBuffer_) {
            cmdBuffer.bindIndexBuffer(indexBuffer_.first->GetBuffer(), indexBuffer_.second, vk::IndexType::eUint32);
        }

        std::get<0>(cameraMatricesUBO_)->Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout_,
            std::get<1>(cameraMatricesUBO_), std::get<2>(cameraMatricesUBO_));
        std::get<0>(worldMatricesUBO_)->Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout_,
            std::get<1>(worldMatricesUBO_), std::get<2>(worldMatricesUBO_));

        for (const auto& ubo : generalUBOs_) {
            std::get<0>(ubo)->Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout_, std::get<1>(ubo),
                                   std::get<2>(ubo));
        }

        for (const auto& ds : generalDescSets_) {
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, ds.second, ds.first,
                                         nullptr);
        }

        cmdBuffer.drawIndexed(indexCount_, instanceCount_, firstIndex_, vertexOffset_, firstInstance_);
        return *this;
    }

}
