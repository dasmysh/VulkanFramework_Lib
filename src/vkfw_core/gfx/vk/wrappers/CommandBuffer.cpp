/**
 * @file   CommandBuffers.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a command buffer class.
 */

// #define VK_USE_PLATFORM_WIN32_KHR

#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk//wrappers/CommandPool.h"
#include "gfx/vk//wrappers/Queue.h"

namespace vkfw_core::gfx {

    CommandBuffer::CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                                 vk::CommandBufferLevel level, std::uint32_t numBuffers)
        : VulkanObjectWrapper{nullptr, name, vk::UniqueCommandBuffer{}}
    // , m_device{ device }
    // , m_queueFamily{ queueFamily }
    {
        spdlog::warn("Command buffers are not fully implemented at the moment.");
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{device->GetCommandPool(queueFamily).GetHandle(), level,
                                                         numBuffers};
        device->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo);
    }

    CommandBuffer::CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                                 vk::UniqueCommandBuffer commandBuffer)
        : CommandBuffer{device->GetHandle(), name, queueFamily, std::move(commandBuffer)}
    {
    }

    void CommandBuffer::Begin(const vk::CommandBufferBeginInfo& beginInfo) const { GetHandle().begin(beginInfo); }

    void CommandBuffer::End() const { GetHandle().end(); }

    CommandBuffer CommandBuffer::beginSingleTimeSubmit(const LogicalDevice* device, std::string_view cmdBufferName,
                                                       std::string_view regionName, const CommandPool& commandPool)
    {
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{commandPool.GetHandle(), vk::CommandBufferLevel::ePrimary, 1};
        auto cmdBuffer = CommandBuffer{device, cmdBufferName, commandPool.GetQueueFamily(),
                          std::move(device->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo)[0])};

        vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
        cmdBuffer.Begin(beginInfo);
        cmdBuffer.BeginLabel(regionName, glm::vec4{1.0f});
        return cmdBuffer;
    }

    void CommandBuffer::endSingleTimeSubmit(const Queue& queue, const CommandBuffer& cmdBuffer,
                                            std::span<vk::Semaphore> waitSemaphores,
                                            std::span<vk::Semaphore> signalSemaphores, const Fence& fence)
    {
        cmdBuffer.EndLabel();
        cmdBuffer.End();
        vk::SubmitInfo submitInfo{static_cast<std::uint32_t>(waitSemaphores.size()),
                                  waitSemaphores.data(),
                                  nullptr,
                                  1,
                                  cmdBuffer.GetHandlePtr(),
            static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data()};
        queue.Submit(submitInfo, fence);
    }

    void CommandBuffer::endSingleTimeSubmitAndWait(const LogicalDevice* device, const Queue& queue,
                                                   const CommandBuffer& cmdBuffer)
    {
        vk::FenceCreateInfo fenceInfo{};
        auto fence = Fence{device->GetHandle(), fmt::format("EndFence {}", cmdBuffer.GetName()), device->GetHandle().createFenceUnique(fenceInfo)};
        endSingleTimeSubmit(queue, cmdBuffer, {}, {}, fence);
        if (auto r = device->GetHandle().waitForFences(fence.GetHandle(), VK_TRUE, vkfw_core::defaultFenceTimeout);
            r != vk::Result::eSuccess) {
            spdlog::error("Error while waiting for fence: {}.", r);
            throw std::runtime_error("Error while waiting for fence.");
        }
    }
#ifndef NDEBUG
    void CommandBuffer::BeginLabel(std::string_view label_name, const glm::vec4& color) const
    {
        GetHandle().beginDebugUtilsLabelEXT(
            vk::DebugUtilsLabelEXT{label_name.data(), std::array<float, 4>{color.r, color.g, color.b, color.a}});
    }

    void CommandBuffer::InsertLabel(std::string_view label_name, const glm::vec4& color) const
    {
        GetHandle().insertDebugUtilsLabelEXT(
            vk::DebugUtilsLabelEXT{label_name.data(), std::array<float, 4>{color.r, color.g, color.b, color.a}});
    }

    void CommandBuffer::EndLabel() const { GetHandle().endDebugUtilsLabelEXT(); }
#else
    void CommandBuffer::BeginLabel(std::string_view label_name, const glm::vec4& color) const {}

    void CommandBuffer::InsertLabel(std::string_view label_name, const glm::vec4& color) const {}

    void CommandBuffer::EndLabel() const {}

#endif

    /*void CommandBuffers::beginSingleTimeSubmit(unsigned int bufferIdx)
    {
        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        vkCmdBuffers_[bufferIdx].begin(beginInfo);
    }

    void CommandBuffers::endSingleTimeSubmit(unsigned bufferIdx, unsigned int queueIndex,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
        vk::Fence fence)
    {
        vkCmdBuffers_[bufferIdx].end();

        vk::SubmitInfo submitInfo{ static_cast<std::uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, 1, &vkCmdBuffers_[bufferIdx], static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device_->GetQueue(queueFamily_, queueIndex).submit(submitInfo, fence);
    }*/
}
