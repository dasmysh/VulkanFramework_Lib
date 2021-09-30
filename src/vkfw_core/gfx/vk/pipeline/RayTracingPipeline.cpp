/**
 * @file   RayTracingPipeline.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.28
 *
 * @brief  Implementation of the ray tracing pipeline.
 */

#include "gfx/vk/pipeline/RayTracingPipeline.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/Shader.h"
#include "gfx/vk/wrappers/PipelineLayout.h"

namespace vkfw_core::gfx {

    RayTracingPipeline::RayTracingPipeline(const LogicalDevice* device, std::string_view name,
                                           std::vector<std::shared_ptr<Shader>>&& shaders)
        : VulkanObjectWrapper{device->GetHandle(), name, vk::UniquePipeline{}}, m_device{device}, m_shaders{shaders}
    {
        ResetShaders(std::move(shaders));

        InitializeShaderBindingTable();
    }

    RayTracingPipeline::~RayTracingPipeline() = default;

    void RayTracingPipeline::CreatePipeline(std::uint32_t maxRecursionDepth, const PipelineLayout& pipelineLayout)
    {

        vk::PipelineLibraryCreateInfoKHR pipelineLibraryInfo{0, nullptr};
        vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
            vk::PipelineCreateFlags{}, m_shaderStages, m_shaderGroups, maxRecursionDepth,
            &pipelineLibraryInfo,      nullptr,        nullptr,        pipelineLayout.GetHandle()};

        if (auto result = m_device->GetHandle().createRayTracingPipelinesKHRUnique(vk::DeferredOperationKHR{},
                                                                                   vk::PipelineCache{}, pipelineInfo);
            result.result != vk::Result::eSuccess) {
            spdlog::error("Could not create ray tracing pipeline.");
            assert(false);
        } else {
            SetHandle(m_device->GetHandle(), std::move(result.value[0]));
        }
        InitializeShaderBindingTable();
    }

    void RayTracingPipeline::ResetShaders(std::vector<std::shared_ptr<Shader>>&& shaders)
    {
        m_shaders = std::move(shaders);
        m_shaderStages.resize(m_shaders.size());
        for (std::size_t i = 0; i < m_shaders.size(); ++i) {
            m_shaders[i]->FillShaderStageInfo(m_shaderStages[i]);
            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eRaygenKHR
                || m_shaderStages[i].stage == vk::ShaderStageFlagBits::eMissKHR
                || m_shaderStages[i].stage == vk::ShaderStageFlagBits::eCallableKHR)
            {
                AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eGeneral, static_cast<std::uint32_t>(i),
                                            VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR);
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eClosestHitKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().closestHitShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().closestHitShader = static_cast<std::uint32_t>(i);
                } else {
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR);
                }
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eAnyHitKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().anyHitShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().anyHitShader = static_cast<std::uint32_t>(i);
                } else {
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR);
                }
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eIntersectionKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().intersectionShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().intersectionShader = static_cast<std::uint32_t>(i);
                    m_shaderGroups.back().type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
                } else {
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR);
                }
            }
        }
    }

    void RayTracingPipeline::InitializeShaderBindingTable()
    {
        const std::uint32_t sbtSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment
                                      * static_cast<std::uint32_t>(m_shaderGroups.size());

        std::vector<std::uint8_t> shaderHandleStorage;
        shaderHandleStorage.resize(sbtSize, 0);

        m_shaderBindingTable = std::make_unique<vkfw_core::gfx::HostBuffer>(
            m_device, fmt::format("{}:ShaderBindingTable", GetName()),
            vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible);

        m_device->GetHandle().getRayTracingShaderGroupHandlesKHR(GetHandle(), 0, static_cast<std::uint32_t>(m_shaderGroups.size()),
                                                                    vk::ArrayProxy<std::uint8_t>(shaderHandleStorage));

        std::vector<std::uint8_t> shaderBindingTable;
        shaderBindingTable.resize(sbtSize, 0);
        auto* data = shaderBindingTable.data();
        {
            auto shaderGroupHandleSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupHandleSize;
            auto shaderGroupBaseAlignment = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment;
            // This part is required, as the alignment and handle size may differ
            for (uint32_t i = 0; i < m_shaderGroups.size(); i++) {
                memcpy(data, shaderHandleStorage.data() + i * static_cast<std::size_t>(shaderGroupHandleSize),
                       shaderGroupHandleSize);
                data += shaderGroupBaseAlignment;
            }
        }

        m_shaderBindingTable->InitializeData(shaderBindingTable);
    }

    template<typename... Args> void RayTracingPipeline::AddShaderGroup(Args&&... args)
    {
        ValidateShaderGroup(m_shaderGroups.back());
        m_shaderGroups.emplace_back(std::forward<Args>(args)...);
    }

    void RayTracingPipeline::ValidateShaderGroup(const vk::RayTracingShaderGroupCreateInfoKHR& shaderGroup)
    {
        if (shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eGeneral
            && (shaderGroup.generalShader == VK_SHADER_UNUSED_KHR
                || shaderGroup.closestHitShader != VK_SHADER_UNUSED_KHR
                || shaderGroup.anyHitShader != VK_SHADER_UNUSED_KHR
                || shaderGroup.intersectionShader != VK_SHADER_UNUSED_KHR)) {
            spdlog::error("Shader groups of type general need a general shader and no others.");
        }

        if (shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup
            && (shaderGroup.generalShader != VK_SHADER_UNUSED_KHR
                || shaderGroup.intersectionShader != VK_SHADER_UNUSED_KHR
                || (shaderGroup.closestHitShader == VK_SHADER_UNUSED_KHR
                    && shaderGroup.anyHitShader == VK_SHADER_UNUSED_KHR))) {
            spdlog::error("Shader groups of type triangle hit need a closest hit or any hit shader and no others.");
        }

        if (shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup
            && (shaderGroup.generalShader != VK_SHADER_UNUSED_KHR
                || shaderGroup.intersectionShader == VK_SHADER_UNUSED_KHR
                || (shaderGroup.closestHitShader == VK_SHADER_UNUSED_KHR
                    && shaderGroup.anyHitShader == VK_SHADER_UNUSED_KHR))) {
            spdlog::error("Shader groups of type procedural hit need an intersection shader and a closest hit or any hit shader and no others.");
        }
    }
}
