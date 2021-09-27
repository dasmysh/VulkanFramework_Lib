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

    Shader::Shader(const std::string & resourceId, const LogicalDevice * device, std::string shaderFilename)
        : Resource{ resourceId, device }
        , VulkanObjectWrapper{device->GetHandle(), fmt::format("Shader:{}", resourceId), vk::UniqueShaderModule{}}
        ,m_shaderFilename{ std::move(shaderFilename) },
        m_type{ vk::ShaderStageFlagBits::eVertex },
        m_strType{ "vertex" }
    {
        if (ends_with(m_shaderFilename, ".frag")) {
            m_type = vk::ShaderStageFlagBits::eFragment;
            m_strType = "fragment";
        } else if (ends_with(m_shaderFilename, ".geom")) {
            m_type = vk::ShaderStageFlagBits::eGeometry;
            m_strType = "geometry";
        } else if (ends_with(m_shaderFilename, ".tesc")) {
            m_type = vk::ShaderStageFlagBits::eTessellationControl;
            m_strType = "tesselation control";
        } else if (ends_with(m_shaderFilename, ".tese")) {
            m_type = vk::ShaderStageFlagBits::eTessellationEvaluation;
            m_strType = "tesselation evaluation";
        } else if (ends_with(m_shaderFilename, ".comp")) {
            m_type = vk::ShaderStageFlagBits::eCompute;
            m_strType = "compute";
        } else if (ends_with(m_shaderFilename, ".mesh")) {
            m_type = vk::ShaderStageFlagBits::eMeshNV;
            m_strType = "mesh";
        } else if (ends_with(m_shaderFilename, ".task")) {
            m_type = vk::ShaderStageFlagBits::eTaskNV;
            m_strType = "task";
        } else if (ends_with(m_shaderFilename, ".rgen")) {
            m_type = vk::ShaderStageFlagBits::eRaygenKHR;
            m_strType = "raygen";
        } else if (ends_with(m_shaderFilename, ".rint")) {
            m_type = vk::ShaderStageFlagBits::eIntersectionKHR;
            m_strType = "intersection";
        } else if (ends_with(m_shaderFilename, ".rahit")) {
            m_type = vk::ShaderStageFlagBits::eAnyHitKHR;
            m_strType = "anyhit";
        } else if (ends_with(m_shaderFilename, ".rchit")) {
            m_type = vk::ShaderStageFlagBits::eClosestHitKHR;
            m_strType = "closesthit";
        } else if (ends_with(m_shaderFilename, ".rmiss")) {
            m_type = vk::ShaderStageFlagBits::eMissKHR;
            m_strType = "miss";
        } else if (ends_with(m_shaderFilename, ".rcall")) {
            m_type = vk::ShaderStageFlagBits::eCallableKHR;
            m_strType = "callable";
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
        shaderStageCreateInfo.setStage(m_type);
        shaderStageCreateInfo.setModule(GetHandle());
        shaderStageCreateInfo.setPName("main");
    }

    void Shader::LoadCompiledShaderFromFile()
    {
        auto filename = FindResourceLocation(m_shaderFilename) + ".spv";
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

        SetHandle(GetDevice()->GetHandle(), GetDevice()->GetHandle().createShaderModuleUnique(moduleCreateInfo));
    }
}
