/**
 * @file   HostTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general host accessible Vulkan image.
 */

#define GLM_FORCE_SWIZZLE

#include "HostTexture.h"

namespace vku::gfx {

    HostTexture::HostTexture(const LogicalDevice* device, const TextureDescriptor& desc,
        const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture{ device, TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent), queueFamilyIndices }
    {
    }

    HostTexture::~HostTexture() = default;

    HostTexture::HostTexture(const HostTexture& rhs) :
        Texture{ rhs.CopyWithoutData() }
    {
        auto texSize = rhs.GetSize();
        auto mipLevels = rhs.GetMipLevels();
        InitializeImage(texSize, mipLevels);
        std::vector<std::uint8_t> tmp(texSize.x * texSize.y * texSize.z);
        for (auto ml = 0U; ml < mipLevels; ++ml) {
            for (auto al = 0U; al < texSize.w; ++al) {
                rhs.DownloadData(ml, al, texSize.xyz, tmp.data());
                UploadData(ml, al, glm::u32vec3(0), texSize.xyz, tmp.data());
            }
        }
    }

    HostTexture& HostTexture::operator=(const HostTexture& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    HostTexture::HostTexture(HostTexture&& rhs) noexcept :
    Texture{ std::move(rhs) }
    {
    }

    HostTexture& HostTexture::operator=(HostTexture&& rhs) noexcept
    {
        this->~HostTexture();
        Texture::operator=(std::move(rhs));
        return *this;
    }

    void HostTexture::InitializeData(const glm::u32vec4& textureSize, std::uint32_t mipLevels, const glm::u32vec4& dataSize, const void* data)
    {
        auto byData = reinterpret_cast<const std::uint8_t*>(data);
        InitializeImage(textureSize, mipLevels);
        for (auto al = 0U; al < dataSize.w; ++al) {
            auto layerData = &byData[dataSize.x * dataSize.y * dataSize.z * al];
            UploadData(0, al, glm::u32vec3(0), dataSize.xyz, layerData);
        }
    }

    void HostTexture::InitializeData(const glm::u32vec4& size, std::uint32_t mipLevels, const void* data)
    {
        InitializeData(size, mipLevels, glm::u32vec4(size.x * GetDescriptor().bytesPP_, size.y, size.z, size.w), data);
    }

    void HostTexture::UploadData(std::uint32_t mipLevel, std::uint32_t arrayLayer,
        const glm::u32vec3& offset, const glm::u32vec3& size, const void* data)
    {
        assert(offset.x + size.x <= GetSize().x);
        assert(offset.y + size.y <= GetSize().y);
        assert(offset.z + size.z <= GetSize().z);
        assert(arrayLayer < GetSize().w);
        assert(mipLevel < GetMipLevels());

        vk::ImageSubresource subresource{ GetValidAspects(), mipLevel, arrayLayer };
        auto layout = GetDevice().getImageSubresourceLayout(GetImage(), subresource);
        GetDeviceMemory().CopyToHostMemory(0, offset, layout, size, data);
    }

    void HostTexture::DownloadData(std::uint32_t mipLevel, std::uint32_t arrayLayer, const glm::u32vec3& size, void* data) const
    {
        vk::ImageSubresource subresource{ GetValidAspects(), mipLevel, arrayLayer };
        auto layout = GetDevice().getImageSubresourceLayout(GetImage(), subresource);

        GetDeviceMemory().CopyFromHostMemory(0, glm::u32vec3(0), layout, size, data);
    }
}
