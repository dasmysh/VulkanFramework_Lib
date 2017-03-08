/**
 * @file   HostTexture.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Declaration of a general host accessible Vulkan image.
 */

#pragma once

#include "main.h"
#include "Texture.h"
#include "core/type_traits.h"

namespace vku { namespace gfx {

    class HostTexture final : public Texture
    {
    public:
        HostTexture(const LogicalDevice* device, const TextureDescriptor& desc,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        ~HostTexture();
        HostTexture(const HostTexture&);
        HostTexture& operator=(const HostTexture&);
        HostTexture(HostTexture&&) noexcept;
        HostTexture& operator=(HostTexture&&) noexcept;

        void InitializeData(const glm::u32vec4& textureSize, uint32_t mipLevels, const glm::u32vec4& dataSize, const void* data);
        void InitializeData(const glm::u32vec4& size, uint32_t mipLevels, const void* data);
        void UploadData(uint32_t mipLevel, uint32_t arrayLayer, const glm::u32vec3& offset, const glm::u32vec3& size, const void* data);
        void DownloadData(uint32_t mipLevel, uint32_t arrayLayer, const glm::u32vec3& size, void* data) const;
    };
}}
