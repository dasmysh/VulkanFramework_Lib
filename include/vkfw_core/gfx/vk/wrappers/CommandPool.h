/**
 * @file   CommandPool.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Wrapper class for a vulkan command pool.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"

namespace vkfw_core::gfx {

    class CommandPool : public VulkanObjectWrapper<vk::UniqueCommandPool>
    {
    public:
        CommandPool() : VulkanObjectWrapper{nullptr, "", vk::UniqueCommandPool{}} {}
        CommandPool(vk::Device device, std::string_view name, vk::UniqueCommandPool commandPool);
    };
}
