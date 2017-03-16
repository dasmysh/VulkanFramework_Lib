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

namespace vku { namespace gfx {

    Texture2D::Texture2D(const std::string& resourceId, const std::string& textureFilename, const LogicalDevice* device) :
        Resource{ textureFilename, device },
        textureFilename_{ textureFilename },
        textureIdx_{ std::numeric_limits<unsigned int>::max() },
        memoryGroup_{ nullptr },
        texture_{ nullptr }
    {
        auto filename = FindResourceLocation(textureFilename_);
        if (!std::experimental::filesystem::exists(filename)) {
            LOG(FATAL) << "Error while loading resource." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: Cannot open texture file.";
            throw file_not_found{ filename };
        }

        stbi_set_flip_vertically_on_load(1);
    }


    Texture2D::Texture2D(const std::string & resourceId, const LogicalDevice * device, const std::string & textureFilename,
        bool useSRGB, QueuedDeviceTransfer & transfer, const std::vector<std::uint32_t>& queueFamilyIndices) :
        Texture2D{ resourceId, textureFilename, device }
    {
        if (stbi_is_hdr(textureFilename.c_str()) != 0) LoadTextureHDR(textureFilename);
        else LoadTextureLDR(textureFilename, useSRGB,
            [this, &transfer, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)
        {
            texturePtr_ = transfer.CreateDeviceTextureWithData(desc, queueFamilyIndices, size, 1, data);
            texture_ = texturePtr_.get();
        });

        InitializeSampler();
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
        if (stbi_is_hdr(textureFilename.c_str()) != 0) LoadTextureHDR(textureFilename);
        else LoadTextureLDR(textureFilename, useSRGB,
            [this, &memGroup, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)
        {
            textureIdx_ = memGroup.AddTextureToGroup(desc, size, 1, queueFamilyIndices);
            memGroup.AddDataToTextureInGroup(textureIdx_, vk::ImageAspectFlagBits::eColor, 1, size.w, size.xyz, data);
            texture_ = memGroup.GetTexture(textureIdx_);
        });

        InitializeSampler();
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
        auto image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, 0);
        if (!image) {
            LOG(FATAL) << "Could not load texture." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: STBI Error.";

            throw stbi_error{};
        }

        unsigned int bytesPP = 4; vk::Format fmt = vk::Format::eR8G8B8A8Unorm;
        std::tie(bytesPP, fmt) = FindFormat(filename, imgChannels, useSRGB);
        TextureDescriptor texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);

        loadFn(glm::u32vec4(imgWidth, imgHeight, 1, 1), texDesc, image);

        stbi_image_free(image);
    }

    void Texture2D::InitializeSampler()
    {
        if (CheckNamedParameterFlag("mirror")) texture_->SampleWrapMirror();
        if (CheckNamedParameterFlag("repeat")) texture_->SampleWrapRepeat();
        if (CheckNamedParameterFlag("clamp")) texture_->SampleWrapClamp();
        if (CheckNamedParameterFlag("mirror-clamp")) texture_->SampleWrapMirrorClamp();
    }

    std::tuple<unsigned, vk::Format> Texture2D::FindFormat(const std::string& filename, int imgChannels, bool useSRGB) const
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
            LOG(FATAL) << "Could not load texture." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Invalid number of texture channels (" << imgChannels << ").";
            throw invalid_texture_channels{ imgChannels };
        }
        return std::make_tuple(bytesPP, fmt);
    }
}}
