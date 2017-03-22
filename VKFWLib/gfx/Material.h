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

    struct Material final
    {
        /** Holds the materials ambient color. */
        glm::vec3 ambient;
        /** Holds the materials diffuse albedo. */
        glm::vec3 diffuse;
        /** Holds the materials specular albedo. */
        glm::vec3 specular;
        /** Holds the materials alpha value. */
        float alpha;
        /** Holds the materials specular exponent. */
        float specularExponent;
        /** Holds the materials index of refraction. */
        float refraction;
        /** Holds the materials diffuse texture. */
        std::shared_ptr<const Texture2D> diffuseTex;
        /** Holds the materials bump texture. */
        std::shared_ptr<const Texture2D> bumpTex;
        /** Holds the materials bump multiplier. */
        float bumpMultiplier;
    };
}
