/**
 * @file   Texture2D.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a 2D texture resource.
 */

#pragma once

#include "main.h"

namespace vku::gfx {

    class QueuedDeviceTransfer;
    class MemoryGroup;
    class DeviceTexture;
    struct TextureDescriptor;

    struct stbi_error {};
    struct invalid_texture_channels { int imgChannels_; };

    class Texture2D final : public Resource
    {
    public:
        Texture2D(const std::string& resourceId, const LogicalDevice* device, const std::string& textureFilename, bool useSRGB,
            QueuedDeviceTransfer& transfer, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB,
            QueuedDeviceTransfer& transfer, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture2D(const std::string& resourceId, const LogicalDevice* device, const std::string& textureFilename, bool useSRGB,
            MemoryGroup& memGroup, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Texture2D(const std::string& textureFilename, const LogicalDevice* device, bool useSRGB,
            MemoryGroup& memGroup, const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ~Texture2D();

    private:
        Texture2D(const std::string& resourceId, const std::string& textureFilename, const LogicalDevice* device_);
        void LoadTextureLDR(const std::string& filename, bool useSRGB,
            const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)>& loadFn);
        void LoadTextureHDR(const std::string& filename,
            const std::function<void(const glm::u32vec4& size, const TextureDescriptor& desc, const void* data)>& loadFn);
        std::tuple<unsigned int, vk::Format> FindFormatLDR(const std::string& filename, int imgChannels, bool useSRGB) const;
        std::tuple<unsigned int, vk::Format> FindFormatHDR(const std::string& filename, int imgChannels) const;

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
