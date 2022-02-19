/**
 * @file   VulkanSyncResources.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.24
 *
 * @brief  Wrapper for vulkan swapchain objects.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"

namespace vkfw_core::gfx {

    class Swapchain : public VulkanObjectWrapper<vk::UniqueSwapchainKHR>
    {
    public:
        Swapchain() : VulkanObjectWrapper{nullptr, "", vk::UniqueSwapchainKHR{}} {}
        Swapchain(vk::Device device, std::string_view name, vk::UniqueSwapchainKHR swapchain)
            : VulkanObjectWrapper{device, name, std::move(swapchain)}
        {
        }
    };

    class Surface : public VulkanObjectWrapper<vk::UniqueSurfaceKHR>
    {
    public:
        Surface() : VulkanObjectWrapper{nullptr, "", vk::UniqueSurfaceKHR{}} {}
        Surface(vk::Device device, Surface&& surface) : VulkanObjectWrapper{std::move(surface)}
        {
            CheckSetName(device);
        }
        Surface(vk::Device device, std::string_view name, vk::UniqueSurfaceKHR surface)
            : VulkanObjectWrapper{device, name, std::move(surface)}
        {
        }
    };
}
