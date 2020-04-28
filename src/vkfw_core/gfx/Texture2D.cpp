/**
 * @file   Texture2D.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a 2D texture resource.
 */

#include "gfx/Texture2D.h"
#include <filesystem>
#include <stb_image.h>
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/textures/HostTexture.h"
#include "gfx/vk/QueuedDeviceTransfer.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "app/VKWindow.h"

namespace vkfw_core::gfx {

    Texture2D::Texture2D(const std::string& textureFilename, bool flipTexture, const LogicalDevice* device)
        : Resource{textureFilename, device},
          m_textureFilename{FindResourceLocation(textureFilename)},
          m_textureIdx{std::numeric_limits<unsigned int>::max()},
          m_memoryGroup{nullptr},
          m_texture{nullptr}
    {
        if (!std::filesystem::exists(m_textureFilename)) {
            spdlog::error(
                "Error while loading resource.\nResourceID: {}\nFilename: {}\nDescription: Cannot open texture file.",
                GetId(), m_textureFilename);
            throw file_not_found{m_textureFilename};
        }

        if (flipTexture) {
            stbi_set_flip_vertically_on_load(1);
        } else {
            stbi_set_flip_vertically_on_load(0);
        }
    }

    Texture2D::Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB,
                         bool flipTexture, QueuedDeviceTransfer& transfer,
                         const std::vector<std::uint32_t>& queueFamilyIndices)
        : Texture2D{textureFilename, flipTexture, device}
    {
        auto loadFn = [this, &transfer, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc,
                                                             void* data) {
            m_texturePtr = transfer.CreateDeviceTextureWithData(desc, queueFamilyIndices, size, 1, data);
            m_texture = m_texturePtr.get();
            stbi_image_free(data);
        };
        if (stbi_is_hdr(m_textureFilename.c_str()) != 0) {
            LoadTextureHDR(m_textureFilename, loadFn);
        } else {
            LoadTextureLDR(m_textureFilename, useSRGB, loadFn);
        }
    }

    Texture2D::Texture2D(const std::string& textureFilename, const LogicalDevice* device,
                         bool useSRGB, bool flipTexture, MemoryGroup& memGroup,
                         const std::vector<std::uint32_t>& queueFamilyIndices)
        :
        Texture2D{ textureFilename, flipTexture, device }
    {
        m_memoryGroup = &memGroup;
        auto loadFn = [this, &memGroup, &queueFamilyIndices](const glm::u32vec4& size, const TextureDescriptor& desc, void* data)
        {
            m_textureIdx = memGroup.AddTextureToGroup(desc, size, 1, queueFamilyIndices);
            glm::u32vec3 dataSize(size.x * memGroup.GetHostTexture(m_textureIdx)->GetDescriptor().m_bytesPP, size.y, size.z);
            memGroup.AddDataToTextureInGroup(m_textureIdx, vk::ImageAspectFlagBits::eColor, 0, 0, dataSize, data,
                                             stbi_image_free);
            m_texture = memGroup.GetTexture(m_textureIdx);
        };
        if (stbi_is_hdr(m_textureFilename.c_str()) != 0) {
            LoadTextureHDR(m_textureFilename, loadFn);
        } else {
            LoadTextureLDR(m_textureFilename, useSRGB, loadFn);
        }
    }

    Texture2D::~Texture2D() = default;

    void Texture2D::LoadTextureLDR(const std::string& filename, bool useSRGB,
        const function_view<void(const glm::u32vec4& size, const TextureDescriptor& desc, void* data)>& loadFn)
    {
        auto imgWidth = 0;
        auto imgHeight = 0;
        auto imgChannels = 0;
        if (stbi_info(filename.c_str(), &imgWidth, &imgHeight, &imgChannels) == 0) {
            spdlog::error(
                "Could not get information from texture (LDR).\nResourceID: {}\nFilename: {}\nDescription: STBI Error.",
                GetId(), filename);
            throw stbi_error{};
        }
        unsigned int bytesPP = 4; vk::Format fmt = vk::Format::eR8G8B8A8Unorm;
        std::tie(bytesPP, fmt) = FindFormat(filename, imgChannels, useSRGB ? FormatProperties::USE_SRGB : FormatProperties::USE_NONE);
        TextureDescriptor texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);
        int requestedChannels = imgChannels;

        auto image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, requestedChannels);
        if (image == nullptr) {
            spdlog::error("Could not load texture (LDR).\nResourceID: {}\nFilename: {}\nDescription: STBI Error.",
                          GetId(), filename);
            throw stbi_error{};
        }

        loadFn(glm::u32vec4(imgWidth, imgHeight, 1, 1), texDesc, image);

        // Data deletion is handled in the loadFn function.
    }

    void Texture2D::LoadTextureHDR(const std::string& filename,
        const function_view<void(const glm::u32vec4& size, const TextureDescriptor& desc, void* data)>& loadFn)
    {
        auto imgWidth = 0;
        auto imgHeight = 0;
        auto imgChannels = 0;
        if (stbi_info(filename.c_str(), &imgWidth, &imgHeight, &imgChannels) == 0) {
            spdlog::error(
                "Could not get information from texture (HDR).\nResourceID: {}\nFilename: {}\nDescription: STBI Error.",
                GetId(), filename);
            throw stbi_error{};
        }

        unsigned int bytesPP = 16; vk::Format fmt = vk::Format::eR32G32B32A32Sfloat;
        std::tie(bytesPP, fmt) = FindFormat(filename, imgChannels, FormatProperties::USE_HDR);
        TextureDescriptor texDesc = TextureDescriptor::SampleOnlyTextureDesc(bytesPP, fmt);
        int requestedChannels = imgChannels;

        auto image = stbi_loadf(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, requestedChannels);
        if (image ==  nullptr) {
            spdlog::error("Could not load texture (HDR).\nResourceID: {}\nFilename: {}\nDescription: STBI Error.",
                          GetId(), filename);
            throw stbi_error{};
        }

        loadFn(glm::u32vec4(imgWidth, imgHeight, 1, 1), texDesc, image);

        // Data deletion is handled in the loadFn function.
    }

    std::pair<unsigned int, vk::Format> Texture2D::FindFormat(const std::string&, int& imgChannels, FormatProperties fmtProps) const
    {
        std::vector<std::pair<unsigned int, vk::Format>> candiateFormats;
        if (imgChannels == 1) {
            if (FormatProperties::USE_SRGB == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(1, vk::Format::eR8Srgb));
            }
            if (FormatProperties::USE_HDR == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(4, vk::Format::eR32Sfloat));
            } else {
                candiateFormats.emplace_back(std::make_pair(1, vk::Format::eR8Unorm));
            }
        }
        if (imgChannels <= 2) {
            if (FormatProperties::USE_SRGB == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(2, vk::Format::eR8G8Srgb));
            }
            if (FormatProperties::USE_HDR == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(8, vk::Format::eR32G32Sfloat));
            } else {
                candiateFormats.emplace_back(std::make_pair(2, vk::Format::eR8G8Unorm));
            }
        }
        if (imgChannels <= 3) {
            if (FormatProperties::USE_SRGB == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(3, vk::Format::eR8G8B8Srgb));
            }
            if (FormatProperties::USE_HDR == fmtProps) {
                constexpr int BYTES_RGB32F = 12;
                candiateFormats.emplace_back(std::make_pair(BYTES_RGB32F, vk::Format::eR32G32B32Sfloat));
            } else {
                candiateFormats.emplace_back(std::make_pair(3, vk::Format::eR8G8B8Unorm));
            }
        }
        if (imgChannels <= 4) {
            if (FormatProperties::USE_SRGB == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(4, vk::Format::eR8G8B8A8Srgb));
            }
            if (FormatProperties::USE_HDR == fmtProps) {
                candiateFormats.emplace_back(std::make_pair(16, vk::Format::eR32G32B32A32Sfloat));
            } else {
                candiateFormats.emplace_back(std::make_pair(4, vk::Format::eR8G8B8A8Unorm));
            }
        }

        auto fmt = GetDevice()->FindSupportedFormat(candiateFormats, vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
        int singleChannelBytes = fmtProps == FormatProperties::USE_HDR ? 4 : 1;
        imgChannels = static_cast<int>(fmt.first) / singleChannelBytes;
        return fmt;
    }
}
