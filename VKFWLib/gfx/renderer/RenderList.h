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

namespace vku::gfx {

    class RenderList final
    {
    public:
        using BufferReference = RenderElement::BufferReference;
        using UBOBinding = RenderElement::UBOBinding;

        inline RenderList(const CameraBase* camera, UBOBinding cameraUBO);

        inline void SetCurrentPipeline(vk::PipelineLayout currentPipelineLayout,
            vk::Pipeline currentOpaquePipeline, vk::Pipeline currentTransparentPipeline);
        inline void SetCurrentGeometry(BufferReference currentVertexBuffer, BufferReference currentIndexBuffer);
        inline void SetCurrentWorldMatrices(UBOBinding currentWorldMatrices);

        inline RenderElement& AddOpaqueElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
            const math::AABB3<float>& boundingBox);
        inline RenderElement& AddTransparentElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
            std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
            const math::AABB3<float>& boundingBox);

        inline void Render(vk::CommandBuffer cmdBuffer);

    private:
        std::vector<RenderElement> opaqueElements_;
        std::vector<RenderElement> transparentElements_;

        const CameraBase* camera_;
        UBOBinding cameraMatricesUBO_;
        vk::PipelineLayout currentPipelineLayout_ = vk::PipelineLayout();
        vk::Pipeline currentOpaquePipeline_ = vk::Pipeline();
        vk::Pipeline currentTransparentPipeline_ = vk::Pipeline();

        BufferReference currentVertexBuffer_ = BufferReference(nullptr, 0);
        BufferReference currentIndexBuffer_ = BufferReference(nullptr, 0);

        UBOBinding currentWorldMatrices_ = UBOBinding(nullptr, 0, 0);
    };

    RenderList::RenderList(const CameraBase* camera, UBOBinding cameraUBO) :
        camera_{ camera },
        cameraMatricesUBO_{ cameraUBO }
    {
    }

    void RenderList::SetCurrentPipeline(vk::PipelineLayout currentPipelineLayout,
        vk::Pipeline currentOpaquePipeline, vk::Pipeline currentTransparentPipeline)
    {
        currentPipelineLayout_ = currentPipelineLayout;
        currentOpaquePipeline_ = currentOpaquePipeline;
        currentTransparentPipeline_ = currentTransparentPipeline;
    }

    void RenderList::SetCurrentGeometry(BufferReference currentVertexBuffer, BufferReference currentIndexBuffer)
    {
        currentVertexBuffer_ = currentVertexBuffer;
        currentIndexBuffer_ = currentIndexBuffer;
    }

    void RenderList::SetCurrentWorldMatrices(UBOBinding currentWorldMatrices)
    {
        currentWorldMatrices_ = currentWorldMatrices;
    }

    vku::gfx::RenderElement& RenderList::AddOpaqueElement(std::uint32_t indexCount, std::uint32_t instanceCount,
        std::uint32_t firstIndex, std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix,
        const math::AABB3<float>& boundingBox)
    {
        auto& result = opaqueElements_.emplace_back(false, currentOpaquePipeline_, currentPipelineLayout_);
        result.BindVertexBuffer(currentVertexBuffer_);
        result.BindIndexBuffer(currentIndexBuffer_);
        result.BindCameraMatricesUBO(cameraMatricesUBO_);
        result.BindWorldMatricesUBO(currentWorldMatrices_);
        result.DrawGeometry(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, viewMatrix, boundingBox);
        return result;
    }

    vku::gfx::RenderElement& RenderList::AddTransparentElement(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::uint32_t vertexOffset, std::uint32_t firstInstance, const glm::mat4& viewMatrix, const math::AABB3<float>& boundingBox)
    {
        auto& result = transparentElements_.emplace_back(true, currentTransparentPipeline_, currentPipelineLayout_);
        result.BindVertexBuffer(currentVertexBuffer_);
        result.BindIndexBuffer(currentIndexBuffer_);
        result.BindCameraMatricesUBO(cameraMatricesUBO_);
        result.BindWorldMatricesUBO(currentWorldMatrices_);
        result.DrawGeometry(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, viewMatrix, boundingBox);
        return result;
    }

    void RenderList::Render(vk::CommandBuffer cmdBuffer)
    {
        std::sort(opaqueElements_.begin(), opaqueElements_.end());
        std::sort(transparentElements_.begin(), transparentElements_.end());

        const RenderElement* lastElement = nullptr;
        for (const auto& re : opaqueElements_) lastElement = &re.DrawElement(cmdBuffer, lastElement);

        lastElement = nullptr;
        for (const auto& re : transparentElements_) lastElement = &re.DrawElement(cmdBuffer, lastElement);
    }

}
