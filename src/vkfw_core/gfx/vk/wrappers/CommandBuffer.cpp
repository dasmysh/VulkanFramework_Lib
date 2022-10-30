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
        , m_device{device}
        , m_queueFamily{ queueFamily }
    {
        spdlog::warn("Command buffers are not fully implemented at the moment.");
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{device->GetCommandPool(queueFamily).GetHandle(), level,
                                                         numBuffers};
        [[maybe_unused]] auto unused = device->GetHandle().allocateCommandBuffersUnique(cmdBufferallocInfo);
    }

    CommandBuffer::CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                                 vk::UniqueCommandBuffer commandBuffer)
        : VulkanObjectWrapper{device->GetHandle(), name, std::move(commandBuffer)}
        , m_device{device}
        , m_queueFamily{queueFamily}
    {
    }

    CommandBuffer::~CommandBuffer()
    {
    }

    void CommandBuffer::Begin(const vk::CommandBufferBeginInfo& beginInfo) const { GetHandle().begin(beginInfo); }

    void CommandBuffer::End() const { GetHandle().end(); }

    std::shared_ptr<Semaphore> CommandBuffer::AddWaitSemaphore(vk::PipelineStageFlags2KHR waitStages)
    {
        auto& waitSemaphore = m_waitSemaphores.emplace_back(std::make_shared<Semaphore>(
            m_device->GetHandle(), fmt::format("{}:WaitSemaphore-{}", GetName(), m_waitSemaphores.size()),
            m_device->GetHandle().createSemaphoreUnique(vk::SemaphoreCreateInfo{})));
        // TODO: At this point we only use binary semaphores, so value is ignored. Needs change for timeline semaphores.
        m_waitSemaphoreSubmitInfos.emplace_back(waitSemaphore->GetHandle(), 0, waitStages);
        return waitSemaphore;
    }

    std::shared_ptr<Fence> CommandBuffer::SubmitToQueue(const Queue& queue,
                                                        std::span<vk::SemaphoreSubmitInfoKHR> waitSemaphores,
                                                        std::span<vk::SemaphoreSubmitInfoKHR> signalSemaphores)
    {
        auto submitFence = m_device->GetResourceReleaser().AddFence(fmt::format("{}:DestroyFence", GetName()));

        m_waitSemaphoreSubmitInfos.resize(m_waitSemaphoreSubmitInfos.size() + waitSemaphores.size());
        for (std::size_t i = 0; i < waitSemaphores.size(); ++i) {
            m_waitSemaphoreSubmitInfos[i + m_waitSemaphores.size()] = waitSemaphores[i];
        }
        for (std::size_t i = 0; i < m_waitSemaphores.size(); ++i) {
            m_device->GetResourceReleaser().AddResource(submitFence, m_waitSemaphores[i]);
        }

        const vk::CommandBufferSubmitInfoKHR commandBufferSubmitInfo{GetHandle()};
        vk::SubmitInfo2KHR submitInfo{vk::SubmitFlagsKHR{}, m_waitSemaphoreSubmitInfos, commandBufferSubmitInfo,
                                      signalSemaphores};
        queue.Submit(submitInfo, submitFence.get());

        m_waitSemaphores.clear();
        m_waitSemaphoreSubmitInfos.clear();
        return submitFence;
    }

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

    std::shared_ptr<Fence> CommandBuffer::endSingleTimeSubmit(const Queue& queue, CommandBuffer& cmdBuffer,
                                                              std::span<vk::SemaphoreSubmitInfoKHR> waitSemaphores,
                                                              std::span<vk::SemaphoreSubmitInfoKHR> signalSemaphores)
    {
        cmdBuffer.EndLabel();
        cmdBuffer.End();
        return cmdBuffer.SubmitToQueue(queue, waitSemaphores, signalSemaphores);
    }

    void CommandBuffer::endSingleTimeSubmitAndWait(const LogicalDevice* device, const Queue& queue,
                                                   CommandBuffer& cmdBuffer)
    {
        auto fence = endSingleTimeSubmit(queue, cmdBuffer, {}, {});
        if (auto r = device->GetHandle().waitForFences(fence->GetHandle(), VK_TRUE, vkfw_core::defaultFenceTimeout);
            r != vk::Result::eSuccess) {
            spdlog::error("Error while waiting for fence: {}.", vk::to_string(r));
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

}
