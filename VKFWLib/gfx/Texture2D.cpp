/**
 * @file   Texture2D.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a 2D texture resource.
 */

#include "Texture2D.h"
#include <boost/filesystem/operations.hpp>
#include <stb_image.h>
#include "gfx/vk/LogicalDevice.h"

namespace vku { namespace gfx {

    Texture2D::Texture2D(const std::string& textureFilename, const LogicalDevice* device) :
        Resource{ textureFilename, device }
    {
        auto filename = FindResourceLocation(GetParameters()[0]);
        if (!boost::filesystem::exists(filename)) {
            LOG(FATAL) << "File \"" << filename.c_str() << L"\" cannot be opened.";
            throw resource_loading_error() << ::boost::errinfo_file_name(filename) << resid_info(getId())
                << errdesc_info("Cannot open texture file.");
        }

        stbi_set_flip_vertically_on_load(1);

        if (stbi_is_hdr(filename.c_str()) != 0) LoadTextureHDR(filename);
        else LoadTextureLDR(filename);

        if (CheckNamedParameterFlag("mirror")) texture_->SampleWrapMirror();
        if (CheckNamedParameterFlag("repeat")) texture_->SampleWrapRepeat();
        if (CheckNamedParameterFlag("clamp")) texture_->SampleWrapClamp();
        if (CheckNamedParameterFlag("mirror-clamp")) texture_->SampleWrapMirrorClamp();
    }


    Texture2D::~Texture2D()
    {
    }

    void Texture2D::LoadTextureLDR(const std::string& filename)
    {
        auto imgWidth = 0, imgHeight = 0, imgChannels = 0;
        auto image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, 0);
        if (!image) {
            LOG(FATAL) << L"Could not load texture \"" << filename.c_str() << L"\".";
            throw resource_loading_error() << ::boost::errinfo_file_name(filename) << resid_info(getId())
                << errdesc_info("Cannot load texture data.");
        }

        vk::ImageCreateInfo imgCreateInfo{ vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D(imgWidth, imgHeight, 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear,
            vk::ImageUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, fqiCount, fqi, vk::ImageLayout::ePreinitialized };

        auto img = device_->GetDevice().createImage(imgCreateInfo);

        vk::MemoryRequirements memRequirements = device_->GetDevice().getImageMemoryRequirements(img);

        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, BufferGroup::FindMemoryType(device_, memRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostCoherent) };
        auto mem = device_->GetDevice().allocateMemory(allocInfo)

        vkBindImageMemory(device, stagingImage, stagingImageMemory, 0);

        auto internalFmt = GL_RGBA8;
        auto fmt = GL_RGBA;
        std::tie(internalFmt, fmt) = FindFormat(filename, imgChannels);

        TextureDescriptor texDesc(4, internalFmt, fmt, GL_UNSIGNED_BYTE);
        texture_ = std::make_unique<GLTexture>(imgWidth, imgHeight, texDesc, image);

        stbi_image_free(image);
    }
}}
