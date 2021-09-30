/**
 * @file   Sampler.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.17
 *
 * @brief  Declaration of a Vulkan sampler object.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class Sampler : public VulkanObjectWrapper<vk::UniqueSampler>
    {
    public:
        Sampler() : VulkanObjectWrapper{nullptr, "", vk::UniqueSampler{}} {}
        Sampler(vk::Device device, std::string_view name, vk::UniqueSampler sampler)
            : VulkanObjectWrapper{device, name, std::move(sampler)}
        {
        }
    };
}
