/**
 * @file   DeviceTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general device memory Vulkan image.
 */

#include "DeviceTexture.h"

namespace vku { namespace gfx {

    DeviceTexture::DeviceTexture(const LogicalDevice* device, uint32_t width, uint32_t height, uint32_t depth,
        const TextureDescriptor& desc, const std::vector<uint32_t>& queueFamilyIndices) :
        Texture{ device, width, height, depth, TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eDeviceLocal), queueFamilyIndices }
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
