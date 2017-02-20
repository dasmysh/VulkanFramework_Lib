/**
 * @file   Texture2D.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Declaration of a 2D texture resource.
 */

#pragma once

#include "main.h"

namespace vku { namespace gfx {

    class Texture2D final : public Resource
    {
    public:
        Texture2D(const std::string& textureFilename, const LogicalDevice* device);
        ~Texture2D();

    private:
        void LoadTextureLDR(const std::string& filename);
        void LoadTextureHDR(const std::string& filename);
    };
}}
