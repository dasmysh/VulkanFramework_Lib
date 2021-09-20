/**
 * @file   CommandPool.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Implementation of a vulkan command pool wrapper.
 */

#include "gfx/vk/wrappers/CommandPool.h"

namespace vkfw_core::gfx {

    CommandPool::CommandPool(vk::CommandPool commandPool)
        : VulkanObjectWrapper{commandPool}
    {
    }

}
