/**
 * @file   HostTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general host accessible Vulkan image.
 */

#include "gfx/vk/textures/HostTexture.h"
#include <glm/gtx/vec_swizzle.hpp>

namespace vkfw_core::gfx {

    HostTexture::HostTexture(const LogicalDevice* device, std::string_view name, const TextureDescriptor& desc,
                             vk::ImageLayout initialLayout, const std::vector<std::uint32_t>& queueFamilyIndices)
        : Texture{device, name,
                  TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eHostVisible
                                              | vk::MemoryPropertyFlagBits::eHostCoherent),
                  initialLayout, queueFamilyIndices}
    {
    }

    HostTexture::~HostTexture() = default;

    HostTexture::HostTexture(const HostTexture& rhs) : Texture{rhs.CopyWithoutData(fmt::format("{}-Copy", rhs.GetName()))}
    {
        auto texSize = rhs.GetSize();
        auto mipLevels = rhs.GetMipLevels();
        InitializeImage(texSize, mipLevels);
        std::vector<std::uint8_t> tmp(static_cast<std::size_t>(texSize.x) * static_cast<std::size_t>(texSize.y) * static_cast<std::size_t>(texSize.z));
        for (auto ml = 0U; ml < mipLevels; ++ml) {
            for (auto al = 0U; al < texSize.w; ++al) {
                rhs.DownloadData(ml, al, glm::xyz(texSize), tmp.data());
                UploadData(ml, al, glm::u32vec3(0), glm::xyz(texSize), tmp.data());
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
        auto byData = reinterpret_cast<const std::uint8_t*>(data); // NOLINT
        InitializeImage(textureSize, mipLevels);
        for (auto al = 0U; al < dataSize.w; ++al) {
            auto layerData = &byData[dataSize.x * dataSize.y * dataSize.z * al]; // NOLINT
            UploadData(0, al, glm::u32vec3(0), glm::xyz(dataSize), layerData);
        }
    }

    void HostTexture::InitializeData(const glm::u32vec4& size, std::uint32_t mipLevels, const void* data)
    {
        InitializeData(size, mipLevels, glm::u32vec4(size.x * GetDescriptor().m_bytesPP, size.y, size.z, size.w), data);
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
