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

    class CommandPool : public VulkanObjectWrapper<vk::CommandPool>
    {
    public:
        CommandPool(vk::CommandPool commandPool);
    };
}
