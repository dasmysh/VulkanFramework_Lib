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

    struct MaterialInfo final
    {
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
        /** Holds the materials diffuse texture. */
        std::string diffuseTextureFilename_;
        /** Holds the materials bump texture. */
        std::string bumpMapFilename_;
        /** Holds the materials bump multiplier. */
        float bumpMultiplier_;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            ar(cereal::make_nvp("objectName", ambient_),
                cereal::make_nvp("serializationID", diffuse_),
                cereal::make_nvp("indexOffset", specular_),
                cereal::make_nvp("numIndices", alpha_),
                cereal::make_nvp("AABB", specularExponent_),
                cereal::make_nvp("AABB", refraction_),
                cereal::make_nvp("AABB", diffuseTextureFilename_),
                cereal::make_nvp("AABB", bumpMapFilename_),
                cereal::make_nvp("material", bumpMultiplier_));
        }
    };
}

CEREAL_CLASS_VERSION(vku::gfx::MaterialInfo, 1)
