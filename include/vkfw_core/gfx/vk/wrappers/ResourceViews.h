/**
 * @file   ImageResources.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.25
 *
 * @brief  Wrapper for the vulkan image related objects.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class ImageView : public VulkanObjectWrapper<vk::UniqueImageView>
    {
    public:
        ImageView() : VulkanObjectWrapper{nullptr, "", vk::UniqueImageView{}} {}
        ImageView(vk::Device device, std::string_view name, vk::UniqueImageView imageView)
            : VulkanObjectWrapper{device, name, std::move(imageView)}
        {
        }
    };

    class BufferView : public VulkanObjectWrapper<vk::UniqueBufferView>
    {
    public:
        BufferView() : VulkanObjectWrapper{nullptr, "", vk::UniqueBufferView{}} {}
        BufferView(vk::Device device, std::string_view name, vk::UniqueBufferView bufferView)
            : VulkanObjectWrapper{device, name, std::move(bufferView)}
        {
        }
    };
}
