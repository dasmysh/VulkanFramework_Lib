/**
 * @file   CommandBuffers.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a command buffer class.
 */

// #define VK_USE_PLATFORM_WIN32_KHR

#include "gfx/vk/CommandBuffers.h"
#include "gfx/vk//wrappers/Queue.h"
#include "gfx/vk//wrappers/CommandPool.h"

namespace vkfw_core::gfx {

    CommandBuffers::CommandBuffers(const LogicalDevice* device, unsigned int queueFamily, vk::CommandBufferLevel level, std::uint32_t numBuffers) :
        m_device{ device },
        m_queueFamily{ queueFamily }
    {
        spdlog::warn("Command buffers are not fully implemented at the moment.");
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ m_device->GetCommandPool(m_queueFamily).GetHandle(), level, numBuffers };
        m_vkCmdBuffers = m_device->GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);
    }

    vk::UniqueCommandBuffer CommandBuffers::beginSingleTimeSubmit(const LogicalDevice* device,
                                                                  const CommandPool& commandPool)
    {
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{commandPool.GetHandle(), vk::CommandBufferLevel::ePrimary, 1};
        auto cmdBuffer = device->GetDevice().allocateCommandBuffersUnique(cmdBufferallocInfo);

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        cmdBuffer[0]->begin(beginInfo);
        return std::move(cmdBuffer[0]);
    }

    void CommandBuffers::endSingleTimeSubmit(const Queue& queue, vk::CommandBuffer cmdBuffer,
                                             const std::vector<vk::Semaphore>& waitSemaphores,
                                             const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence)
    {
        cmdBuffer.end();
        vk::SubmitInfo submitInfo{
            static_cast<std::uint32_t>(waitSemaphores.size()),   waitSemaphores.data(),  nullptr, 1, &cmdBuffer,
            static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data()};
        queue.Submit(submitInfo, fence);
    }

    void CommandBuffers::endSingleTimeSubmitAndWait(const LogicalDevice* device, const Queue& queue,
                                                    vk::CommandBuffer cmdBuffer)
    {
        vk::FenceCreateInfo fenceInfo{};
        auto fence = device->GetDevice().createFenceUnique(fenceInfo);
        endSingleTimeSubmit(queue, cmdBuffer, {}, {}, fence.get());
        if (auto r = device->GetDevice().waitForFences(fence.get(), VK_TRUE, vkfw_core::defaultFenceTimeout); r != vk::Result::eSuccess) {
            spdlog::error("Error while waiting for fence: {}.", r);
            throw std::runtime_error("Error while waiting for fence.");
        }
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
