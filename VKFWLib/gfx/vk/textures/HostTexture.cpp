/**
 * @file   HostTexture.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Implementation of a general host accessible Vulkan image.
 */

#include "HostTexture.h"

namespace vku { namespace gfx {

    HostTexture::HostTexture(const LogicalDevice* device, uint32_t width, uint32_t height, uint32_t depth,
        const TextureDescriptor& desc, const std::vector<uint32_t>& queueFamilyIndices) :
        Texture{ device, width, height, depth,
            TextureDescriptor(desc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent), queueFamilyIndices }
    {
    }

    HostTexture::~HostTexture() = default;

    HostTexture::HostTexture(const HostTexture& rhs) :
        Texture{ rhs.CopyWithoutData() }
    {
        std::vector<int8_t> tmp(GetSize());
        rhs.DownloadData(tmp);
        InitializeData(tmp);
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

    void HostTexture::InitializeData(size_t bufferSize, size_t dataSize, const void* data)
    {
        InitializeBuffer(bufferSize);
        UploadData(0, dataSize, data);
    }

    void HostTexture::InitializeData(size_t size, const void* data)
    {
        InitializeData(size, size, data);
    }

    void HostTexture::UploadData(size_t offset, size_t size, const void* data)
    {
        if (offset + size > GetSize()) {
            std::vector<int8_t> tmp(offset);
            DownloadData(tmp);
            InitializeBuffer(offset + size);
            UploadDataInternal(0, offset, tmp.data());
        }

        UploadDataInternal(offset, size, data);
    }

    void HostTexture::DownloadData(size_t size, void* data) const
    {
        vk::SubresourceLayout layout;

        vk::ImageSubresource subresource{ vk::ImageAspectFlagBits::eColor, 0, 0 };
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.mipLevel = 0;
        subresource.arrayLayer = 0;

        VkSubresourceLayout stagingImageLayout;
        vkGetImageSubresourceLayout(device, stagingImage, &subresource, &stagingImageLayout);
        GetDeviceMemory().CopyFromHostMemory(0, size, data);
    }

    void HostTexture::UploadDataInternal(size_t offset, size_t size, const void* data) const
    {
        GetDeviceMemory().CopyToHostMemory(offset, size, data);
    }
}}
