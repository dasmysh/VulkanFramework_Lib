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
}
