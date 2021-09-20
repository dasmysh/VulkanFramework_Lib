/**
 * @file   Queue.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Implementation of a vulkan queue wrapper.
 */

#include "gfx/vk/wrappers/Queue.h"

namespace vkfw_core::gfx {

    Queue::Queue(vk::Queue queue, const CommandPool& commandPool)
        : VulkanObjectWrapper{queue}, m_commandPool{commandPool}
    {
    }

#ifndef NDEBUG
    void Queue::BeginLabel(std::string_view label_name, const glm::vec4& color) const
    {
        GetHandle().beginDebugUtilsLabelEXT(
            vk::DebugUtilsLabelEXT{label_name.data(), std::array<float, 4>{color.r, color.g, color.b, color.a}});
    }

    void Queue::InsertLabel(std::string_view label_name, const glm::vec4& color) const
    {
        GetHandle().insertDebugUtilsLabelEXT(
            vk::DebugUtilsLabelEXT{label_name.data(), std::array<float, 4>{color.r, color.g, color.b, color.a}});
    }

    void Queue::EndLabel() const { GetHandle().endDebugUtilsLabelEXT(); }
#else
    void Queue::BeginLabel(std::string_view label_name, const glm::vec4& color) const {}

    void Queue::InsertLabel(std::string_view label_name, const glm::vec4& color) const {}

    void Queue::EndLabel() const {}
#endif

    void Queue::Submit(const vk::SubmitInfo& submitInfo, vk::Fence fence) const
    {
        GetHandle().submit(submitInfo, fence);
    }

    vk::Result Queue::Present(const vk::PresentInfoKHR& presentInfo) const { return GetHandle().presentKHR(presentInfo); }

    void Queue::WaitIdle() const { GetHandle().waitIdle(); }

}
