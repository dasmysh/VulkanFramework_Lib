/**
 * @file   ShaderManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.15
 *
 * @brief  Contains the definition of ShaderManager.
 */

#pragma once

#include "gfx/vk/Shader.h"

namespace vkfw_core {

    class ShaderManager final : public ResourceManager<gfx::Shader>
    {
    public:
        explicit ShaderManager(const gfx::LogicalDevice* device);
        ShaderManager(const ShaderManager&);
        ShaderManager& operator=(const ShaderManager&);
        ShaderManager(ShaderManager&&) noexcept;
        ShaderManager& operator=(ShaderManager&&) noexcept;
        ~ShaderManager() override;

    };
}
