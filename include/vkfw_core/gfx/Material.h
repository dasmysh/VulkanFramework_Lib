/**
 * @file   Material.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.14
 *
 * @brief  Declaration of the material class.
 */

#pragma once

#include "main.h"

#include <glm/vec3.hpp>

namespace vkfw_core::gfx {

    class Texture2D;
    class LogicalDevice;
    class MemoryGroup;

    struct MaterialInfo final
    {
        /** Holds the materials name. */
        std::string m_materialName;
        /** Holds the materials ambient color. */
        glm::vec3 m_ambient = glm::vec3{1.0f};
        /** Holds the materials diffuse albedo. */
        glm::vec3 m_diffuse = glm::vec3{1.0f};
        /** Holds the materials specular albedo. */
        glm::vec3 m_specular = glm::vec3{1.0f};
        /** Holds the materials alpha value. */
        float m_alpha = 1.0f;
        /** Holds the materials specular exponent. */
        float m_specularExponent = 0.0f;
        /** Holds the materials index of refraction. */
        float m_refraction = 1.0f;
        /** Holds the materials diffuse texture file name. */
        std::string m_diffuseTextureFilename;
        /** Holds the materials bump map file name. */
        std::string m_bumpMapFilename;
        /** Holds the materials bump multiplier. */
        float m_bumpMultiplier = 1.0f;
        /** Holds whether this material has an alpha channel. */
        bool m_hasAlpha = false;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t) // NOLINT
        {
            ar(cereal::make_nvp("ambientColor", m_ambient),
                cereal::make_nvp("diffuseColor", m_diffuse),
                cereal::make_nvp("specularColor", m_specular),
                cereal::make_nvp("alpha", m_alpha),
                cereal::make_nvp("specularExponent", m_specularExponent),
                cereal::make_nvp("refractionIndex", m_refraction),
                cereal::make_nvp("diffuseTextureFilename", m_diffuseTextureFilename),
                cereal::make_nvp("bumpMapFilename", m_bumpMapFilename),
                cereal::make_nvp("bumpMultiplier", m_bumpMultiplier),
                cereal::make_nvp("hasAlpha", m_hasAlpha));
        }
    };

    struct Material final
    {
        Material();
        Material(const MaterialInfo* materialInfo, const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Material(const Material&);
        Material& operator=(const Material&);
        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;
        ~Material();

        /** Holds the material information. */
        const MaterialInfo* m_materialInfo = nullptr;
        /** Holds the materials diffuse texture. */
        std::shared_ptr<Texture2D> m_diffuseTexture;
        /** Holds the materials bump map. */
        std::shared_ptr<Texture2D> m_bumpMap;
    };
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::MaterialInfo, 1)
