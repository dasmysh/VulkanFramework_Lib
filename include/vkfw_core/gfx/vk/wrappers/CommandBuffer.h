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
        CommandBuffer(vk::Device device, std::string_view name, vk::UniqueCommandBuffer commandBuffer)
            : VulkanObjectWrapper{device, name, std::move(commandBuffer)}
        {
        }

        CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                      vk::CommandBufferLevel level, std::uint32_t numBuffers);

        void Begin(const vk::CommandBufferBeginInfo& beginInfo) const;
        void End() const;

        void BeginLabel(std::string_view label_name, const glm::vec4& color) const;
        void InsertLabel(std::string_view label_name, const glm::vec4& color) const;
        void EndLabel() const;

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
        CommandBuffer(const LogicalDevice* device, std::string_view name, vk::UniqueCommandBuffer commandBuffer);
        // /** Holds the device. */
        // const LogicalDevice* m_device;
        // /** Holds the queue family for this buffers. */
        // unsigned int m_queueFamily;
    };
}
