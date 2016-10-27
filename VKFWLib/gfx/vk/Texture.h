/**
 * @file   Texture.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.26
 *
 * @brief  Declaration of a texture object for Vulkan.
 */

#pragma once

#include "main.h"

namespace vku {
    namespace gfx {

        struct TextureDescriptor
        {
            /** Holds the bytes per pixel of the format. */
            unsigned int bytesPP_;
            /** Holds the textures format. */
            vk::Format format_;
        };

        class Texture
        {
        public:
            Texture();
            Texture(const Texture&);
            Texture(Texture&&) noexcept;
            Texture& operator=(const Texture&);
            Texture& operator=(Texture&&) noexcept;
            ~Texture();

        private:
        };
    }
}