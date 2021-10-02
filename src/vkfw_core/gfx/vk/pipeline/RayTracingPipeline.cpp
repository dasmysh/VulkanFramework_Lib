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

    std::size_t ShaderTypeToTypeGroupIndex(vk::ShaderStageFlagBits type)
    {
        switch (type) {
        case vk::ShaderStageFlagBits::eRaygenKHR: return 0;
        case vk::ShaderStageFlagBits::eAnyHitKHR: return 2;
        case vk::ShaderStageFlagBits::eClosestHitKHR: return 2;
        case vk::ShaderStageFlagBits::eMissKHR: return 1;
        case vk::ShaderStageFlagBits::eIntersectionKHR: return 2;
        case vk::ShaderStageFlagBits::eCallableKHR: return 3;
        default: break;
        }
        return 4;
    }

    RayTracingPipeline::RayTracingPipeline(const LogicalDevice* device, std::string_view name,
                                           std::vector<std::shared_ptr<Shader>>&& shaders)
        : VulkanObjectWrapper{device->GetHandle(), name, vk::UniquePipeline{}}, m_device{device}, m_shaders{shaders}
    {
        ResetShaders(std::move(shaders));
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
                m_shaderGroupIndexesByType[ShaderTypeToTypeGroupIndex(m_shaderStages[i].stage)].push_back(
                    static_cast<std::uint32_t>(m_shaderGroups.size()));
                AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eGeneral, static_cast<std::uint32_t>(i),
                                            VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR);
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eClosestHitKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().closestHitShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().closestHitShader = static_cast<std::uint32_t>(i);
                } else {
                    m_shaderGroupIndexesByType[ShaderTypeToTypeGroupIndex(m_shaderStages[i].stage)].push_back(
                        static_cast<std::uint32_t>(m_shaderGroups.size()));
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR);
                }
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eAnyHitKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().anyHitShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().anyHitShader = static_cast<std::uint32_t>(i);
                } else {
                    m_shaderGroupIndexesByType[ShaderTypeToTypeGroupIndex(m_shaderStages[i].stage)].push_back(
                        static_cast<std::uint32_t>(m_shaderGroups.size()));
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR);
                }
            } else if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eIntersectionKHR) {
                if (!m_shaderGroups.empty() && m_shaderGroups.back().type != vk::RayTracingShaderGroupTypeKHR::eGeneral
                    && m_shaderGroups.back().intersectionShader == VK_SHADER_UNUSED_KHR) {
                    m_shaderGroups.back().intersectionShader = static_cast<std::uint32_t>(i);
                    m_shaderGroups.back().type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
                } else {
                    m_shaderGroupIndexesByType[ShaderTypeToTypeGroupIndex(m_shaderStages[i].stage)].push_back(
                        static_cast<std::uint32_t>(m_shaderGroups.size()));
                    AddShaderGroup(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                   static_cast<std::uint32_t>(i), VK_SHADER_UNUSED_KHR);
                }
            }
        }
    }

    void RayTracingPipeline::InitializeShaderBindingTable()
    {
        auto shaderGroupBaseAlignment = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment;
        {
            // TODO: allow shader record parameters. [10/1/2021 Sebastian Maisch]
            // Parameters are memory after the shader group handle (see shaderGroupHandleSize in rt properties)
            // that can contain any variables that can fit into a uniform buffer.
            // max amount is maxShaderGroupStride - shaderGroupHandleSize (see both in rt properties)
            const std::uint32_t sbtSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment
                                          * static_cast<std::uint32_t>(m_shaderGroups.size());

            std::vector<std::uint8_t> shaderHandleStorage;
            shaderHandleStorage.resize(sbtSize, 0);

            m_shaderBindingTable = std::make_unique<vkfw_core::gfx::HostBuffer>(
                m_device, fmt::format("{}:ShaderBindingTable", GetName()),
                vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eHostVisible);

            m_device->GetHandle().getRayTracingShaderGroupHandlesKHR(GetHandle(), 0,
                                                                     static_cast<std::uint32_t>(m_shaderGroups.size()),
                                                                     vk::ArrayProxy<std::uint8_t>(shaderHandleStorage));

            std::vector<std::uint8_t> shaderBindingTable;
            shaderBindingTable.resize(sbtSize, 0);
            auto* data = shaderBindingTable.data();
            {
                // This part is required, as the alignment and handle size may differ
                auto shaderGroupHandleSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupHandleSize;

                for (std::size_t i_type = 0; i_type < m_shaderGroupIndexesByType.size(); ++i_type) {
                    m_shaderGroupTypeOffset[i_type] = data - shaderBindingTable.data();
                    for (std::size_t i_group = 0; i_group < m_shaderGroupIndexesByType[i_type].size(); ++i_group) {
                        memcpy(data,
                               shaderHandleStorage.data()
                                   + m_shaderGroupIndexesByType[i_type][i_group]
                                         * static_cast<std::size_t>(shaderGroupHandleSize),
                               shaderGroupHandleSize);
                        data += shaderGroupBaseAlignment;
                    }
                }
                // for (uint32_t i = 0; i < m_shaderGroups.size(); i++) {
                //     memcpy(data, shaderHandleStorage.data() + i * static_cast<std::size_t>(shaderGroupHandleSize),
                //            shaderGroupHandleSize);
                //     data += shaderGroupBaseAlignment;
                // }
            }

            m_shaderBindingTable->InitializeData(shaderBindingTable);
        }

        // TODO: might be (slightly) faster to have a device memory SBT. [10/1/2021 Sebastian Maisch]


        vk::DeviceSize shaderBindingTableSize = shaderGroupBaseAlignment * m_shaderGroups.size();

        auto sbtDeviceAddress = m_shaderBindingTable->GetDeviceAddress().deviceAddress;
        auto rayGenDeviceAddress = sbtDeviceAddress + m_shaderGroupTypeOffset[0];
        m_sbtDeviceAddressRegions[0] =
            vk::StridedDeviceAddressRegionKHR{rayGenDeviceAddress, shaderBindingTableSize, shaderBindingTableSize};
        auto missDeviceAddress = sbtDeviceAddress + sbtDeviceAddress + m_shaderGroupTypeOffset[1];
        m_sbtDeviceAddressRegions[1] =
            vk::StridedDeviceAddressRegionKHR{missDeviceAddress, shaderBindingTableSize, shaderBindingTableSize};
        auto hitDeviceAddress = sbtDeviceAddress + sbtDeviceAddress + m_shaderGroupTypeOffset[2];
        m_sbtDeviceAddressRegions[2] =
            vk::StridedDeviceAddressRegionKHR{hitDeviceAddress, shaderBindingTableSize, shaderBindingTableSize};
        auto callableDeviceAddress = sbtDeviceAddress + sbtDeviceAddress + m_shaderGroupTypeOffset[3];
        m_sbtDeviceAddressRegions[3] =
            vk::StridedDeviceAddressRegionKHR{callableDeviceAddress, shaderBindingTableSize, shaderBindingTableSize};
    }

    template<typename... Args> void RayTracingPipeline::AddShaderGroup(Args&&... args)
    {
        if (!m_shaderGroups.empty()) { ValidateShaderGroup(m_shaderGroups.back()); }
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
