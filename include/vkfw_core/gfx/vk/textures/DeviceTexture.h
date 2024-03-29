/**
 * @file   DeviceTexture.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Declaration of a general device memory Vulkan image.
 */

#pragma once

#include "main.h"
#include "Texture.h"

namespace vkfw_core::gfx {

    class DeviceTexture final : public Texture
    {
    public:
        DeviceTexture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
                      vk::ImageLayout initialLayout,
                      const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ~DeviceTexture() override;
        DeviceTexture(const DeviceTexture&) = delete;
        DeviceTexture& operator=(const DeviceTexture&) = delete;
        DeviceTexture(DeviceTexture&&) noexcept;
        DeviceTexture& operator=(DeviceTexture&&) noexcept;
    };
}
