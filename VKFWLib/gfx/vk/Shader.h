/**
 * @file   Shader.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Declaration of the shader class.
 */

#pragma once

#include "main.h"

namespace vku {
    namespace gfx {

        class Shader final : public Resource
        {
        public:
            Shader(const std::string& shaderFilename, const LogicalDevice* device);
            Shader(const Shader&);
            Shader& operator=(const Shader&);
            Shader(Shader&&) noexcept;
            Shader& operator=(Shader&&) noexcept;
            ~Shader();

            void FillShaderStageInfo(vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo) const;

        private:
            void LoadCompiledShaderFromFile();

            /** Holds the compiled shader. */
            vk::ShaderModule shaderModule_;
            /** Holds the shaders type. */
            vk::ShaderStageFlagBits type_;
            /** Holds the shaders type as a string. */
            std::string strType_;
        };
    }
}
