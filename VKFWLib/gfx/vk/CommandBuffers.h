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

namespace vku { namespace gfx {

    class CommandBuffers
    {
    public:
        CommandBuffers(const LogicalDevice* device, unsigned int queueFamily, vk::CommandBufferLevel level, uint32_t numBuffers);

        void beginSingleTimeSubmit(unsigned int bufferIdx);
        void endSingleTimeSubmit(unsigned int bufferIdx, unsigned int queueIndex,
            const std::vector<vk::Semaphore>& waitSemaphores = std::vector<vk::Semaphore>{},
            const std::vector<vk::Semaphore>& signalSemaphores = std::vector<vk::Semaphore>{},
            vk::Fence fence = vk::Fence());

    private:
        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the queue family for this buffers. */
        unsigned int queueFamily_;
        /** Holds the vulkan command buffer objects. */
        std::vector<vk::CommandBuffer> vkCmdBuffers_;
    };
}}
