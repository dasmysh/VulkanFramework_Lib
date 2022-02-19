/**
 * @file   PipelineLayout.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.27
 *
 * @brief  Wrapper class for a vulkan pipeline layout.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class PipelineLayout : public VulkanObjectWrapper<vk::UniquePipelineLayout>
    {
    public:
        PipelineLayout(vk::Device device, std::string_view name, vk::UniquePipelineLayout pipelineLayout)
            : VulkanObjectWrapper{device, name, std::move(pipelineLayout)}
        {
        }
    };
}
