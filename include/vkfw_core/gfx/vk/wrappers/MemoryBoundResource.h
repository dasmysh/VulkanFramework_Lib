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

        std::tuple<vk::AccessFlags2KHR, vk::PipelineStageFlags2KHR, unsigned int> GetPreviousAccess() const
        {
            return std::make_tuple(m_prevAccess, m_prevPipelineStages, m_prevQueueFamily);
        }

        void SetAccess(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages, unsigned int queueFamily)
        {
            m_prevAccess = access;
            m_prevPipelineStages = pipelineStages;
            m_prevQueueFamily = queueFamily;
        }

        static constexpr unsigned int INVALID_QUEUE_FAMILY = static_cast<unsigned int>(-1);

    protected:
        /** Holds the device. */
        const LogicalDevice* m_device;

    private:
        /** The last accesses access flags. */
        vk::AccessFlags2KHR m_prevAccess = vk::AccessFlagBits2KHR::eNone;
        /** The last accesses pipeline stage(s). */
        vk::PipelineStageFlags2KHR m_prevPipelineStages = vk::PipelineStageFlagBits2KHR::eNone;
        /** The last accesses queue family. */
        unsigned int m_prevQueueFamily = INVALID_QUEUE_FAMILY;
    };
}
