/**
 * @file   CommandBuffers.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a command buffer class.
 */

// #define VK_USE_PLATFORM_WIN32_KHR

#include "gfx/vk/CommandBuffers.h"

namespace vku::gfx {

    CommandBuffers::CommandBuffers(const LogicalDevice* device, unsigned int queueFamily, vk::CommandBufferLevel level, std::uint32_t numBuffers) :
        device_{ device },
        queueFamily_{ queueFamily }
    {
        spdlog::warn("Command buffers are not fully implemented at the moment.");
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device_->GetCommandPool(queueFamily_) , level, numBuffers };
        vkCmdBuffers_ = device_->GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);
    }

    vk::UniqueCommandBuffer CommandBuffers::beginSingleTimeSubmit(const LogicalDevice* device, unsigned int queueFamily)
    {
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device->GetCommandPool(queueFamily) , vk::CommandBufferLevel::ePrimary, 1 };
        auto cmdBuffer = device->GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        cmdBuffer[0]->begin(beginInfo);
        return std::move(cmdBuffer[0]);
    }

    void CommandBuffers::endSingleTimeSubmit(const LogicalDevice* device, vk::CommandBuffer cmdBuffer,
        unsigned int queueFamily, unsigned int queueIndex, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence)
    {
        cmdBuffer.end();

        vk::SubmitInfo submitInfo{ static_cast<std::uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, 1, &cmdBuffer, static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device->GetQueue(queueFamily, queueIndex).submit(submitInfo, fence);
    }

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
