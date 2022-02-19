/**
 * @file   VertexInputResources.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.25
 *
 * @brief  Helper classes for creating pipeline barriers for vertex input resources.
 */

#pragma once

#include "PipelineBarriers.h"
#include "main.h"

namespace vkfw_core::gfx {

    class CommandBuffer;

    struct BufferDescription
    {
        Buffer* m_buffer = nullptr;
        std::size_t m_offset = 0;
    };

    class VertexInputResources
    {
    public:
        VertexInputResources(const LogicalDevice* device, std::uint32_t firstVertexBinding,
                             std::span<BufferDescription> vertexBuffers, const BufferDescription& indexBuffer,
                             vk::IndexType indexType);

        void BindBarrier(CommandBuffer& cmdBuffer);
        void Bind(CommandBuffer& cmdBuffer);

    private:
        /** Pipeline barrier for using this vertex input resources. */
        gfx::PipelineBarrier m_barrier;

        /** Holds the vertex buffers to bind. */
        std::vector<vk::Buffer> m_vertexBuffers;
        /** Holds the first binding index to update. */
        std::uint32_t m_firstVertexBinding = 0;
        /** Holds the offsets of the vertex buffers. */
        std::vector<std::size_t> m_vertexBufferOffsets;

        /** Holds the vertex buffer to bind. */
        vk::Buffer m_indexBuffer = nullptr;
        /** Holds the offsets of the index buffer. */
        std::size_t m_indexBufferOffset;
        /** Holds the index type. */
        vk::IndexType m_indexType;

        // TODO: more resources for instance buffers?

        /** Flags the vertex input resources to skip the next bind barrier since it was already set from the framebuffer. */
        unsigned int m_skipNextBindBarriers = 0;
    };
}
