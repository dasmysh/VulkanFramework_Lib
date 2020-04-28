/**
 * @file   CommandBuffers.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a command buffer class.
 */

#pragma once

#include "main.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx {

    class CommandBuffers
    {
    public:
        CommandBuffers(const LogicalDevice* device, unsigned int queueFamily, vk::CommandBufferLevel level, std::uint32_t numBuffers);

        static vk::UniqueCommandBuffer beginSingleTimeSubmit(const LogicalDevice* device, unsigned int queueFamily);
        static void endSingleTimeSubmit(const LogicalDevice* device, vk::CommandBuffer cmdBuffer,
            unsigned int queueFamily, unsigned int queueIndex,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence());

    private:
        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the queue family for this buffers. */
        unsigned int m_queueFamily;
        /** Holds the vulkan command buffer objects. */
        std::vector<vk::UniqueCommandBuffer> m_vkCmdBuffers;
    };
}
