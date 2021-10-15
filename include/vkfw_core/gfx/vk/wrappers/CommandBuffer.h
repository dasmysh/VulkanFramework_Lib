/**
 * @file   CommandBuffers.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a command buffer class.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "gfx/vk/LogicalDevice.h"
#include "main.h"
#include "VulkanSyncResources.h"

namespace vkfw_core::gfx {

    class Queue;

    class CommandBuffer : public VulkanObjectWrapper<vk::UniqueCommandBuffer>
    {
    public:
        CommandBuffer() : VulkanObjectWrapper{nullptr, "", vk::UniqueCommandBuffer{}} {}
        CommandBuffer(vk::Device device, std::string_view name, unsigned int queueFamily, vk::UniqueCommandBuffer commandBuffer)
            : VulkanObjectWrapper{device, name, std::move(commandBuffer)}, m_queueFamily{queueFamily}
        {
        }

        CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                      vk::CommandBufferLevel level, std::uint32_t numBuffers);

        void Begin(const vk::CommandBufferBeginInfo& beginInfo) const;
        void End() const;

        void BeginLabel(std::string_view label_name, const glm::vec4& color) const;
        void InsertLabel(std::string_view label_name, const glm::vec4& color) const;
        void EndLabel() const;

        static std::vector<CommandBuffer> Initialize(vk::Device device, std::string_view name, unsigned int queueFamily,
                                                     std::vector<vk::UniqueCommandBuffer>&& commandBuffers)
        {
            std::vector<CommandBuffer> result;
            for (std::size_t i = 0; i < commandBuffers.size(); ++i) {
                result.emplace_back(device, fmt::format("{}-{}", name, i), queueFamily, std::move(commandBuffers[i]));
            }
            return result;
        }

        unsigned int GetQueueFamily() const { return m_queueFamily; }

        static CommandBuffer beginSingleTimeSubmit(const LogicalDevice* device, std::string_view cmdBufferName,
                                                   std::string_view regionName, const CommandPool& commandPool);
        static void
        endSingleTimeSubmit(const Queue& queue, const CommandBuffer& cmdBuffer,
                            std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                            std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{},
                            const Fence& fence = Fence{});
        static void endSingleTimeSubmitAndWait(const LogicalDevice* device, const Queue& queue,
                                               const CommandBuffer& cmdBuffer);

    private:
        CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                      vk::UniqueCommandBuffer commandBuffer);

        /** Holds the queue family for this command buffer. */
        unsigned int m_queueFamily = static_cast<unsigned int>(-1);
    };
}
