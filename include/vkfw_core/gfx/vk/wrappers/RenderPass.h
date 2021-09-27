/**
 * @file   RenderPass.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.25
 *
 * @brief  Wrapper for the vulkan render pass object.
 */

#pragma once

#include "main.h"
#include "VulkanObjectWrapper.h"

namespace vkfw_core::gfx {

    class RenderPass : public VulkanObjectWrapper<vk::UniqueRenderPass>
    {
    public:
        RenderPass() : VulkanObjectWrapper{nullptr, "", vk::UniqueRenderPass{}} {}
        RenderPass(vk::Device device, std::string_view name, vk::UniqueRenderPass renderPass)
            : VulkanObjectWrapper{device, name, std::move(renderPass)}
        {
        }
    };
}
