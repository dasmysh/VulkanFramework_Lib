/**
 * @file   Texture2D.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a 2D texture resource.
 */

#define GLM_FORCE_SWIZZLE

#include "Texture2D.h"
#include <filesystem>
#include <stb_image.h>
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/textures/HostTexture.h"
#include "vk/QueuedDeviceTransfer.h"
#include "vk/memory/MemoryGroup.h"
#include "app/VKWindow.h"

namespace vku::gfx {

    Texture2D::Texture2D(const std::string& resourceId, const std::string& textureFilename, const LogicalDevice* device) :
        Resource{ textureFilename, device },
        textureFilename_{ FindResourceLocation(textureFilename) },
        textureIdx_{ std::numeric_limits<unsigned int>::max() },
        memoryGroup_{ nullptr },
        texture_{ nullptr }
    {
        if (!std::experimental::filesystem::exists(textureFilename_)) {
            LOG(ERROR) << "Error while loading resource." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << textureFilename_ << std::endl
                << "Description: Cannot open texture file.";
            throw file_not_found{ textureFilename_ };
        }

        stbi_set_flip_vertically_on_load(1);
    }


    Texture2D::Texture2D(const std::string & resourceId, const LogicalDevice * device, const std::string & textureFilename,
        bool useSRGB, QueuedDeviceTransfer & transfer, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture2D{ resourceId, textureFilename, device }
    {
        auto loadFn = [this, &transfer, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)
        {
            texturePtr_ = transfer.CreateDeviceTextureWithData(desc, queueFamilyIndices, size, 1, data);
            texture_ = texturePtr_.get();
            stbi_image_free(const_cast<void*>(data));
        };
        if (stbi_is_hdr(textureFilename_.c_str()) != 0) LoadTextureHDR(textureFilename_, loadFn);
        else LoadTextureLDR(textureFilename_, useSRGB, loadFn);
    }

    Texture2D::Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB,
        QueuedDeviceTransfer& transfer, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture2D{ textureFilename, device, textureFilename, useSRGB, transfer, queueFamilyIndices }
    {
    }

    Texture2D::Texture2D(const std::string & resourceId, const LogicalDevice * device, const std::string & textureFilename,
        bool useSRGB, MemoryGroup & memGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture2D{ resourceId, textureFilename, device }
    {
        memoryGroup_ = &memGroup;
        auto loadFn = [this, &memGroup, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)
        {
            textureIdx_ = memGroup.AddTextureToGroup(desc, size, 1, queueFamilyIndices);
            glm::u32vec3 dataSize(size.x * memGroup.GetHostTexture(textureIdx_)->GetDescriptor().bytesPP_, size.y, size.z);
            memGroup.AddDataToTextureInGroup(textureIdx_, vk::ImageAspectFlagBits::eColor, 0, 0, dataSize, data, stbi_image_free);
            texture_ = memGroup.GetTexture(textureIdx_);
        };
        if (stbi_is_hdr(textureFilename_.c_str()) != 0) LoadTextureHDR(textureFilename_, loadFn);
        else LoadTextureLDR(textureFilename_, useSRGB, loadFn);
    }

    Texture2D::Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB,
        MemoryGroup& memGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture2D{ textureFilename, device, textureFilename, useSRGB, memGroup, queueFamilyIndices }
    {
    }

    Texture2D::~Texture2D()
    {
    }

    void Texture2D::LoadTextureLDR(const std::string& filename, bool useSRGB,
        const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)>& loadFn)
    {
        auto imgWidth = 0, imgHeight = 0, imgChannels = 0;
        if (!stbi_info(filename.c_str(), &imgWidth, &imgHeight, &imgChannels)) {
            LOG(ERROR) << "Could not get information from texture (LDR)." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: STBI Error.";

            throw stbi_error{};
        }
        unsigned int bytesPP = 4; vk::Format fmt = vk::Format::eR8G8B8A8Unorm;
        std::tie(bytesPP, fmt) = FindFormatLDR(filename, imgChannels, useSRGB);
        TextureDescriptor texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);

        int requestedChannels = imgChannels;
        while (!texDesc.IsFormatSupported(GetDevice()->GetPhysicalDevice())) {
            requestedChannels += 1;
            std::tie(bytesPP, fmt) = FindFormatLDR(filename, requestedChannels, useSRGB);
            texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);
        }

        auto image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, requestedChannels);
        if (!image) {
            LOG(ERROR) << "Could not load texture (LDR)." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: STBI Error.";

            throw stbi_error{};
        }

        loadFn(glm::u32vec4(imgWidth, imgHeight, 1, 1), texDesc, image);

        // This is done in the loadFn or after copying data...
        // stbi_image_free(image);
    }

    void Texture2D::LoadTextureHDR(const std::string& filename,
        const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)>& loadFn)
    {
        auto imgWidth = 0, imgHeight = 0, imgChannels = 0;
        if (!stbi_info(filename.c_str(), &imgWidth, &imgHeight, &imgChannels)) {
            LOG(ERROR) << "Could not get information from texture (LDR)." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: STBI Error.";

            throw stbi_error{};
        }

        unsigned int bytesPP = 16; vk::Format fmt = vk::Format::eR32G32B32A32Sfloat;
        std::tie(bytesPP, fmt) = FindFormatHDR(filename, imgChannels);
        TextureDescriptor texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);

        int requestedChannels = imgChannels;
        while (!texDesc.IsFormatSupported(GetDevice()->GetPhysicalDevice())) {
            requestedChannels += 1;
            std::tie(bytesPP, fmt) = FindFormatHDR(filename, requestedChannels);
            texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);
        }

        auto image = stbi_loadf(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, requestedChannels);
        if (!image) {
            LOG(ERROR) << "Could not load texture (HDR)." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: STBI Error.";

            throw stbi_error{};
        }

        loadFn(glm::u32vec4(imgWidth, imgHeight, 1, 1), texDesc, image);

        // This is done in the loadFn or after copying data...
        // stbi_image_free(image);
    }

    std::tuple<unsigned, vk::Format> Texture2D::FindFormatLDR(const std::string& filename, int imgChannels, bool useSRGB) const
    {
        auto useSRGBFormat = (useSRGB && GetDevice()->GetWindowCfg().useSRGB_);
        auto fmt = vk::Format::eR8G8B8A8Unorm;
        unsigned int bytesPP = 4;
        switch (imgChannels) {
        case 1: fmt = useSRGBFormat ? vk::Format::eR8Srgb : vk::Format::eR8Unorm; bytesPP = 1; break;
        case 2: fmt = useSRGBFormat ? vk::Format::eR8G8Srgb : vk::Format::eR8G8Unorm; bytesPP = 2; break;
        case 3: fmt = useSRGBFormat ? vk::Format::eR8G8B8Srgb : vk::Format::eR8G8B8Unorm; bytesPP = 3; break;
        case 4: fmt = useSRGBFormat ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm; bytesPP = 4; break;
        default:
            LOG(ERROR) << "Could not load texture." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Invalid number of texture channels (" << imgChannels << ").";
            throw invalid_texture_channels{ imgChannels };
        }
        return std::make_tuple(bytesPP, fmt);
    }

    std::tuple<unsigned, vk::Format> Texture2D::FindFormatHDR(const std::string& filename, int imgChannels) const
    {
        auto fmt = vk::Format::eR32G32B32A32Sfloat;
        unsigned int bytesPP = 16;
        switch (imgChannels) {
        case 1: fmt = vk::Format::eR32Sfloat; bytesPP = 4; break;
        case 2: fmt = vk::Format::eR32G32Sfloat; bytesPP = 8; break;
        case 3: fmt = vk::Format::eR32G32B32Sfloat; bytesPP = 12; break;
        case 4: fmt = vk::Format::eR32G32B32A32Sfloat; bytesPP = 16; break;
        default:
            LOG(ERROR) << "Could not load texture." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Invalid number of texture channels (" << imgChannels << ").";
            throw invalid_texture_channels{ imgChannels };
        }
        return std::make_tuple(bytesPP, fmt);
    }
}
