/**
 * @file   Material.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.14
 *
 * @brief  Declaration of the material class.
 */

#pragma once

#include "main.h"

namespace vku::gfx {

    class Texture2D;
    class LogicalDevice;
    class MemoryGroup;

    struct MaterialInfo final
    {
        /** Holds the materials name. */
        std::string materialName_;
        /** Holds the materials ambient color. */
        glm::vec3 ambient_;
        /** Holds the materials diffuse albedo. */
        glm::vec3 diffuse_;
        /** Holds the materials specular albedo. */
        glm::vec3 specular_;
        /** Holds the materials alpha value. */
        float alpha_;
        /** Holds the materials specular exponent. */
        float specularExponent_;
        /** Holds the materials index of refraction. */
        float refraction_;
        /** Holds the materials diffuse texture file name. */
        std::string diffuseTextureFilename_;
        /** Holds the materials bump map file name. */
        std::string bumpMapFilename_;
        /** Holds the materials bump multiplier. */
        float bumpMultiplier_;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            ar(cereal::make_nvp("ambientColor", ambient_),
                cereal::make_nvp("diffuseColor", diffuse_),
                cereal::make_nvp("specularColor", specular_),
                cereal::make_nvp("alpha", alpha_),
                cereal::make_nvp("specularExponent", specularExponent_),
                cereal::make_nvp("refractionIndex", refraction_),
                cereal::make_nvp("diffuseTextureFilename", diffuseTextureFilename_),
                cereal::make_nvp("bumpMapFilename", bumpMapFilename_),
                cereal::make_nvp("bumpMultiplier", bumpMultiplier_));
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
        const MaterialInfo* materialInfo_;
        /** Holds the materials diffuse texture. */
        std::shared_ptr<Texture2D> diffuseTexture_;
        /** Holds the materials bump map. */
        std::shared_ptr<Texture2D> bumpMap_;
    };
}

CEREAL_CLASS_VERSION(vku::gfx::MaterialInfo, 1)
