/**
 * @file   CommandBuffers.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a command buffer class.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "ReleaseableResource.h"
#include "gfx/vk/LogicalDevice.h"
#include "main.h"
#include "VulkanSyncResources.h"
#include <variant>

namespace vkfw_core::gfx {

    class Queue;

    class CommandBuffer : public VulkanObjectWrapper<vk::UniqueCommandBuffer>, public ReleaseableResource
    {
    public:
        CommandBuffer(const LogicalDevice* device)
            : VulkanObjectWrapper{nullptr, "", vk::UniqueCommandBuffer{}}
            , m_device{device}
        {
        }
        CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                      vk::UniqueCommandBuffer commandBuffer);
        CommandBuffer(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                      vk::CommandBufferLevel level, std::uint32_t numBuffers);

        CommandBuffer(CommandBuffer&& rhs) noexcept = default;
        CommandBuffer& operator=(CommandBuffer&& rhs) noexcept = default;

        ~CommandBuffer();

        void Begin(const vk::CommandBufferBeginInfo& beginInfo) const;
        void End() const;

        [[nodiscard]] std::shared_ptr<Semaphore> AddWaitSemaphore();

        std::shared_ptr<Fence>
        SubmitToQueue(const Queue& queue, std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                      std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{});

        void BeginLabel(std::string_view label_name, const glm::vec4& color) const;
        void InsertLabel(std::string_view label_name, const glm::vec4& color) const;
        void EndLabel() const;

        [[nodiscard]] static std::vector<CommandBuffer>
        Initialize(const LogicalDevice* device, std::string_view name, unsigned int queueFamily,
                   std::vector<vk::UniqueCommandBuffer>&& commandBuffers)
        {
            std::vector<CommandBuffer> result;
            for (std::size_t i = 0; i < commandBuffers.size(); ++i) {
                result.emplace_back(device, fmt::format("{}-{}", name, i), queueFamily, std::move(commandBuffers[i]));
            }
            return result;
        }

        [[nodiscard]] unsigned int GetQueueFamily() const { return m_queueFamily; }

        [[nodiscard]] static CommandBuffer beginSingleTimeSubmit(const LogicalDevice* device,
                                                                 std::string_view cmdBufferName,
                                                                 std::string_view regionName,
                                                                 const CommandPool& commandPool);
        [[nodiscard]] static std::shared_ptr<Fence>
        endSingleTimeSubmit(const Queue& queue, CommandBuffer& cmdBuffer,
                            std::span<vk::Semaphore> waitSemaphores = std::span<vk::Semaphore>{},
                            std::span<vk::Semaphore> signalSemaphores = std::span<vk::Semaphore>{});
        static void endSingleTimeSubmitAndWait(const LogicalDevice* device, const Queue& queue,
                                               CommandBuffer& cmdBuffer);

    private:
        /** The device to be able to wait for the fence. */
        const LogicalDevice* m_device = nullptr;
        /** Holds the queue family for this command buffer. */
        unsigned int m_queueFamily = static_cast<unsigned int>(-1);
        /** Holds the wait semaphores used on submit. */
        std::vector<std::shared_ptr<Semaphore>> m_waitSemaphores;
    };
}
