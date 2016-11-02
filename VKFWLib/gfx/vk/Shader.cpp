/**
 * @file   Shader.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Implementation of the shader class.
 */

#include "Shader.h"
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include "LogicalDevice.h"

namespace vku { namespace gfx {
    
    Shader::Shader(const std::string& shaderFilename, gfx::LogicalDevice* device) :
        Resource{ shaderFilename, device },
        type_{ vk::ShaderStageFlagBits::eVertex },
        strType_{ "vertex" }
    {
        auto shaderDefinition = GetParameters();
        if (boost::ends_with(shaderDefinition[0], ".frag")) {
            type_ = vk::ShaderStageFlagBits::eFragment;
            strType_ = "fragment";
        }
        else if (boost::ends_with(shaderDefinition[0], ".geom")) {
            type_ = vk::ShaderStageFlagBits::eGeometry;
            strType_ = "geometry";
        }
        else if (boost::ends_with(shaderDefinition[0], ".tesc")) {
            type_ = vk::ShaderStageFlagBits::eTessellationControl;
            strType_ = "tesselation control";
        }
        else if (boost::ends_with(shaderDefinition[0], ".tese")) {
            type_ = vk::ShaderStageFlagBits::eTessellationEvaluation;
            strType_ = "tesselation evaluation";
        }
        else if (boost::ends_with(shaderDefinition[0], ".comp")) {
            type_ = vk::ShaderStageFlagBits::eCompute;
            strType_ = "compute";
        }

        LoadCompiledShaderFromFile();
    }

    Shader::~Shader()
    {
        if (shaderModule_) device_->GetDevice().destroyShaderModule(shaderModule_);
        shaderModule_ = vk::ShaderModule();
    }

    void Shader::FillShaderStageInfo(vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo) const
    {
        shaderStageCreateInfo.setStage(type_);
        shaderStageCreateInfo.setModule(shaderModule_);
        shaderStageCreateInfo.setPName("main");
    }

    void Shader::LoadCompiledShaderFromFile()
    {
        std::ifstream file(GetFilename(), std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            LOG(FATAL) << "Could not open shader file (" << GetFilename() << ").";
            throw std::runtime_error("Could not open shader file.");
        }
        auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        vk::ShaderModuleCreateInfo moduleCreateInfo{ vk::ShaderModuleCreateFlags(), fileSize, reinterpret_cast<uint32_t*>(buffer.data) };

        shaderModule_ = device_->GetDevice().createShaderModule(moduleCreateInfo);
    }
}}
