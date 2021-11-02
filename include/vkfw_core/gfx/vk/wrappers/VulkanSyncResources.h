/**
 * @file   VulkanSyncResources.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.24
 *
 * @brief  Wrapper for vulkan synchronization objects.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"
#include "ReleaseableResource.h"

namespace vkfw_core::gfx {

    class LogicalDevice;

    class Fence : public VulkanObjectWrapper<vk::UniqueFence>
    {
    public:
        Fence() : VulkanObjectWrapper{nullptr, "", vk::UniqueFence{}} {}
        Fence(vk::Device device, std::string_view name, vk::UniqueFence fence)
            : VulkanObjectWrapper{device, name, std::move(fence)}
        {
        }

        void Wait(const LogicalDevice* device, std::uint64_t timeout) const;
        bool IsSignaled(const LogicalDevice* device) const;
        void Reset(const LogicalDevice* device, std::string_view name = "");
    };

    class Semaphore : public VulkanObjectWrapper<vk::UniqueSemaphore>, public ReleaseableResource
    {
    public:
        Semaphore() : VulkanObjectWrapper{nullptr, "", vk::UniqueSemaphore{}} {}
        Semaphore(vk::Device device, std::string_view name, vk::UniqueSemaphore semaphore)
            : VulkanObjectWrapper{device, name, std::move(semaphore)}
        {
        }
    };
}
