/**
 * @file   Texture2D.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a 2D texture resource.
 */

#pragma once

#include "main.h"

#include <glm/gtc/type_precision.hpp>

namespace vku::gfx {

    class QueuedDeviceTransfer;
    class MemoryGroup;
    class DeviceTexture;
    struct TextureDescriptor;

    struct stbi_error final : public std::exception {
        stbi_error() : std::exception{ "STBI Error." } {}
    };
    struct invalid_texture_channels final : public std::exception {
        explicit invalid_texture_channels(int imgChannels) : std::exception{ "Invalid number of image channels." }, imgChannels_{ imgChannels } {}
        int imgChannels_;
    };

    class Texture2D final : public Resource
    {
    public:
        Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB, bool flipTexture,
                  QueuedDeviceTransfer& transfer,
                  const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture2D(const std::string& textureFilename, const LogicalDevice* device,
                  bool useSRGB, bool flipTexture, MemoryGroup& memGroup,
                  const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture2D(const Texture2D&) = delete;
        Texture2D(Texture2D&&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
        Texture2D& operator=(Texture2D&&) = delete;
        ~Texture2D() override;

        [[nodiscard]] const DeviceTexture& GetTexture() const { return *texture_; }

    private:
        enum class FormatProperties {
            USE_NONE,
            USE_SRGB,
            USE_HDR
        };

        Texture2D(const std::string& textureFilename, bool flipTexture, const LogicalDevice* device_);
        void LoadTextureLDR(const std::string& filename, bool useSRGB,
            const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, void* data)>& loadFn);
        void LoadTextureHDR(const std::string& filename,
            const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, void* data)>& loadFn);
        std::pair<unsigned int, vk::Format> FindFormat(const std::string& filename, int& imgChannels, FormatProperties fmtProps) const;

        /** Holds the texture file name. */
        std::string textureFilename_;
        /** Holds the unique pointer to the texture used. */
        std::unique_ptr<DeviceTexture> texturePtr_;
        /** Holds the index in the memory group to the texture used. */
        unsigned int textureIdx_;
        /** Holds the memory group containing the texture. */
        MemoryGroup* memoryGroup_;
        /** Holds the pointer to the texture used. */
        DeviceTexture* texture_;
    };
}
