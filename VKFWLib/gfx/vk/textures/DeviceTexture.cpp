/**
 * @file   DeviceTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general device memory Vulkan image.
 */

#include "DeviceTexture.h"

namespace vku { namespace gfx {

    DeviceTexture::DeviceTexture(const LogicalDevice* device, const TextureDescriptor& desc,
        const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture{ device, TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eDeviceLocal), queueFamilyIndices }
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
}}
