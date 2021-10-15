/**
 * @file   MemoryBoundResource.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.11
 *
 * @brief  Base class for memory bound resources with access control.
 */

#pragma once

#include "CommandBuffer.h"
#include "PipelineBarriers.h"
#include "main.h"
#include <concepts>

namespace vkfw_core::gfx {

    class SingleResourcePipelineBarrier;
    class PipelineBarrier;
    class LogicalDevice;

    class MemoryBoundResource
    {
    public:
        MemoryBoundResource(const LogicalDevice* device) : m_device{device} {};
        MemoryBoundResource(const MemoryBoundResource& rhs) = delete;
        MemoryBoundResource& operator=(const MemoryBoundResource& rhs) = delete;
        MemoryBoundResource(MemoryBoundResource&& rhs) noexcept
            : m_device{rhs.m_device}
            , m_prevAccess{rhs.m_prevAccess}
            , m_prevPipelineStages{rhs.m_prevPipelineStages}
            , m_prevQueueFamily{rhs.m_prevQueueFamily}
        {
        }
        MemoryBoundResource& operator=(MemoryBoundResource&& rhs) noexcept
        {
            m_device = rhs.m_device;
            SetAccess(rhs.m_prevAccess, rhs.m_prevPipelineStages, rhs.m_prevQueueFamily);
            return *this;
        }

        std::tuple<vk::AccessFlags, vk::PipelineStageFlags, unsigned int> GetPreviousAccess() const
        {
            return std::make_tuple(m_prevAccess, m_prevPipelineStages, m_prevQueueFamily);
        }

        void SetAccess(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages, unsigned int queueFamily)
        {
            m_prevAccess = access;
            m_prevPipelineStages = pipelineStages;
            m_prevQueueFamily = queueFamily;
        }

    protected:
        /** Holds the device. */
        const LogicalDevice* m_device;

    private:
        /** The last accesses access flags. */
        vk::AccessFlags m_prevAccess = vk::AccessFlags{};
        /** The last accesses pipeline stage(s). */
        vk::PipelineStageFlags m_prevPipelineStages = vk::PipelineStageFlagBits::eTopOfPipe;
        /** The last accesses queue family. */
        unsigned int m_prevQueueFamily = 0;
    };


    template<class T> class BufferAccessor
    {
    public:
        // TODO: move operators
        T Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages, SingleResourcePipelineBarrier& barrier);
        T Get(vk::AccessFlags access, vk::PipelineStageFlags pipelineStages, PipelineBarrier& barrier);

        private:
            T m_resource;
    };

    class AccessableBufferResource : public MemoryBoundResource
    {
    public:
        BufferAccessor<vk::Buffer> GetAccess();
    };
}
