/**
 * @file   DeviceTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general device memory Vulkan image.
 */

#include "gfx/vk/textures/DeviceTexture.h"

namespace vkfw_core::gfx {

    DeviceTexture::DeviceTexture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
                                 vk::ImageLayout initialLayout, const std::vector<std::uint32_t>& queueFamilyIndices)
        : Texture{device, name, TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eDeviceLocal), initialLayout,
                  queueFamilyIndices}
    {
    }

    DeviceTexture::~DeviceTexture() = default;

    DeviceTexture::DeviceTexture(DeviceTexture&& rhs) noexcept :
    Texture{ std::move(rhs) }
    {
    }

    DeviceTexture& DeviceTexture::operator=(DeviceTexture&& rhs) noexcept
    {
        this->~DeviceTexture();
        Texture::operator=(std::move(rhs));
        return *this;
    }
}
