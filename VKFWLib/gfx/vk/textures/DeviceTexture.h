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

namespace vku { namespace gfx {

    class DeviceTexture final : public Texture
    {
    public:
        DeviceTexture(const LogicalDevice* device, uint32_t width, uint32_t height, uint32_t depth,
            const TextureDescriptor& desc,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        ~DeviceTexture();
        DeviceTexture(const DeviceTexture&) = delete;
        DeviceTexture& operator=(const DeviceTexture&) = delete;
        DeviceTexture(DeviceTexture&&) noexcept;
        DeviceTexture& operator=(DeviceTexture&&) noexcept;
    };
}}
