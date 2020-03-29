/**
 * @file   Shader.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Implementation of the shader class.
 */

#include "gfx/vk/Shader.h"
#include <fstream>
#include "gfx/vk/LogicalDevice.h"
#include "core/string_algorithms.h"

namespace vkfw_core::gfx {

    Shader::Shader(const std::string & resourceId, const LogicalDevice * device, std::string shaderFilename) :
        Resource{ resourceId, device },
        shaderFilename_{ std::move(shaderFilename) },
        type_{ vk::ShaderStageFlagBits::eVertex },
        strType_{ "vertex" }
    {
        if (ends_with(shaderFilename_, ".frag")) {
            type_ = vk::ShaderStageFlagBits::eFragment;
            strType_ = "fragment";
        }
        else if (ends_with(shaderFilename_, ".geom")) {
            type_ = vk::ShaderStageFlagBits::eGeometry;
            strType_ = "geometry";
        }
        else if (ends_with(shaderFilename_, ".tesc")) {
            type_ = vk::ShaderStageFlagBits::eTessellationControl;
            strType_ = "tesselation control";
        }
        else if (ends_with(shaderFilename_, ".tese")) {
            type_ = vk::ShaderStageFlagBits::eTessellationEvaluation;
            strType_ = "tesselation evaluation";
        }
        else if (ends_with(shaderFilename_, ".comp")) {
            type_ = vk::ShaderStageFlagBits::eCompute;
            strType_ = "compute";
        }

        LoadCompiledShaderFromFile();
    }

    Shader::Shader(const std::string& shaderFilename, const LogicalDevice* device) :
        Shader{ shaderFilename, device, shaderFilename }
    {
    }

    Shader::~Shader() = default;

    void Shader::FillShaderStageInfo(vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo) const
    {
        shaderStageCreateInfo.setStage(type_);
        shaderStageCreateInfo.setModule(*shaderModule_);
        shaderStageCreateInfo.setPName("main");
    }

    void Shader::LoadCompiledShaderFromFile()
    {
        auto filename = FindResourceLocation(shaderFilename_) + ".spv";
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            spdlog::error("Could not open shader file ({}).", filename);
            throw std::runtime_error("Could not open shader file.");
        }
        auto fileSize = static_cast<std::size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        vk::ShaderModuleCreateInfo moduleCreateInfo{ vk::ShaderModuleCreateFlags(), fileSize, reinterpret_cast<std::uint32_t*>(buffer.data()) }; // NOLINT

        shaderModule_ = GetDevice()->GetDevice().createShaderModuleUnique(moduleCreateInfo);
    }
}
