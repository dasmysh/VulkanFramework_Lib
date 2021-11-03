/**
 * @file   VertexInputResources.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.25
 *
 * @brief  Implementation the vertex input resources helper classes.
 */

#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/PipelineBarriers.h"
#include "gfx/vk/wrappers/VertexInputResources.h"

namespace vkfw_core::gfx {

    VertexInputResources::VertexInputResources(const LogicalDevice* device, std::uint32_t firstVertexBinding,
                                               std::span<BufferDescription> vertexBuffers,
                                               const BufferDescription& indexBuffer, vk::IndexType indexType)
        : m_barrier{device}
        , m_vertexBuffers{vertexBuffers.size(), nullptr}
        , m_firstVertexBinding{firstVertexBinding}
        , m_vertexBufferOffsets(vertexBuffers.size(), 0)
        , m_indexBuffer{indexBuffer.m_buffer ? indexBuffer.m_buffer->GetBuffer(
                            vk::AccessFlagBits2KHR::eIndexRead, vk::PipelineStageFlagBits2KHR::eVertexInput, m_barrier)
                                             : nullptr}
        , m_indexBufferOffset{indexBuffer.m_offset}
        , m_indexType{indexType}
    {
        for (std::size_t i = 0; i < m_vertexBuffers.size(); ++i) {
            m_vertexBuffers[i] = vertexBuffers[i].m_buffer->GetBuffer(
                vk::AccessFlagBits2KHR::eVertexAttributeRead, vk::PipelineStageFlagBits2KHR::eVertexInput, m_barrier);
            m_vertexBufferOffsets[i] = vertexBuffers[i].m_offset;
        }
    }

    void VertexInputResources::BindBarrier(CommandBuffer& cmdBuffer)
    {
        m_barrier.Record(cmdBuffer);
        m_skipNextBindBarriers += 1;
    }

    void VertexInputResources::Bind(CommandBuffer& cmdBuffer)
    {
        if (m_skipNextBindBarriers == 0) {
            m_barrier.Record(cmdBuffer);
        } else {
            m_skipNextBindBarriers -= 1;
        }

        cmdBuffer.GetHandle().bindVertexBuffers(m_firstVertexBinding, m_vertexBuffers, m_vertexBufferOffsets);
        cmdBuffer.GetHandle().bindIndexBuffer(m_indexBuffer, m_indexBufferOffset, m_indexType);
    }
}
