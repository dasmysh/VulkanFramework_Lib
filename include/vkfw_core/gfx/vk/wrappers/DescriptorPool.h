/**
 * @file   DescriptorPool.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.23
 *
 * @brief  Wrapper class for a vulkan descriptor pool.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class DescriptorPool : public VulkanObjectWrapper<vk::UniqueDescriptorPool>
    {
    public:
        DescriptorPool() : VulkanObjectWrapper{nullptr, "", vk::UniqueDescriptorPool{}} {}
        DescriptorPool(vk::Device device, std::string_view name, vk::UniqueDescriptorPool descriptorPool)
            : VulkanObjectWrapper{device, name, std::move(descriptorPool)}
        {
        }
    };
}
