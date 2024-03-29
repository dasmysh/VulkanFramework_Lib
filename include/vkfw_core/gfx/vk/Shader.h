/**
 * @file   Shader.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Declaration of the shader class.
 */

#pragma once

#include "main.h"
#include "gfx/vk/wrappers/VulkanObjectWrapper.h"

namespace vkfw_core::gfx {

    class Shader final : public Resource, public VulkanObjectWrapper<vk::UniqueShaderModule>
    {
    public:
        Shader(const std::string& shaderFilename, const LogicalDevice* device);
        Shader(const std::string& resourceId, const LogicalDevice* device, std::string shaderFilename);
        Shader(const Shader&);
        Shader& operator=(const Shader&);
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader&&) noexcept;
        ~Shader() override;

        void FillShaderStageInfo(vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo) const;

    private:
        void LoadCompiledShaderFromFile();

        /** Holds the shader filename. */
        std::string m_shaderFilename;
        /** Holds the shaders type. */
        vk::ShaderStageFlagBits m_type;
        /** Holds the shaders type as a string. */
        std::string m_strType;
    };
}
