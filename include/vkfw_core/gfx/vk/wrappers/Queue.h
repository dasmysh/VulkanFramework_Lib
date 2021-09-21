/**
 * @file   Queue.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Wrapper for a vulkan queue.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"
#include "CommandPool.h"
#include <glm/vec4.hpp>

namespace vkfw_core::gfx {

    class Queue : public VulkanObjectWrapper<vk::Queue>
    {
    public:
        Queue(vk::Queue queue, const CommandPool& commandPool);

        void Submit(const vk::SubmitInfo& submitInfo, vk::Fence fence) const;
        [[nodiscard]] vk::Result Present(const vk::PresentInfoKHR& presentInfo) const;
        void WaitIdle() const;

        void BeginLabel(std::string_view label_name, const glm::vec4& color) const;
        void InsertLabel(std::string_view label_name, const glm::vec4& color) const;
        void EndLabel() const;

        const CommandPool& GetCommandPool() const { return m_commandPool; }

    private:
        CommandPool m_commandPool;
    };

    class QueueRegion
    {
    public:
        QueueRegion(const Queue& queue, std::string_view region_name, const glm::vec4& color = glm::vec4{1.0})
            : m_queue{queue}
        {
            m_queue.BeginLabel(region_name, color);
        }

        ~QueueRegion() { m_queue.EndLabel(); }

    private:
        const Queue& m_queue;
    };
}

#define QUEUE_REGION(queue, ...) \
const vkfw_core::gfx::QueueRegion UNIQUENAME(queue_region_) { queue, __VA_ARGS__ }
