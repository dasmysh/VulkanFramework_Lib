/**
 * @file   CommandBuffers.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a command buffer class.
 */

#include "CommandBuffers.h"

namespace vku { namespace gfx {

    CommandBuffers::CommandBuffers(const LogicalDevice* device, unsigned int queueFamily, vk::CommandBufferLevel level, uint32_t numBuffers):
        device_{ device },
        queueFamily_{ queueFamily }
    {
        LOG(WARNING) << "Command buffers are not fully implemented at the moment.";
        vk::CommandBufferAllocateInfo cmdBufferallocInfo{ device_->GetCommandPool(queueFamily_) , level, numBuffers };
        vkCmdBuffers_ = device_->GetDevice().allocateCommandBuffers(cmdBufferallocInfo);
    }

    void CommandBuffers::beginSingleTimeSubmit(unsigned int bufferIdx)
    {
        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        vkCmdBuffers_[bufferIdx].begin(beginInfo);
    }

    void CommandBuffers::endSingleTimeSubmit(unsigned bufferIdx, unsigned int queueIndex,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
        vk::Fence fence)
    {
        vkCmdBuffers_[bufferIdx].end();

        vk::SubmitInfo submitInfo{ static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            nullptr, 1, &vkCmdBuffers_[bufferIdx], static_cast<uint32_t>(signalSemaphores.size()), signalSemaphores.data() };
        device_->GetQueue(queueFamily_, queueIndex).submit(submitInfo, fence);
    }
}}
