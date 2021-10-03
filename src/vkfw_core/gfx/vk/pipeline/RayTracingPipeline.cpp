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
        auto shaderGroupHandleSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupHandleSize;
        auto shaderGroupHandleSizeAligned = m_device->CalculateSBTHandleAlignment(shaderGroupHandleSize);
        {
            // TODO: allow shader record parameters. [10/1/2021 Sebastian Maisch]
            // Parameters are memory after the shader group handle (see shaderGroupHandleSize in rt properties)
            // that can contain any variables that can fit into a uniform buffer.
            // max amount is maxShaderGroupStride - shaderGroupHandleSize (see both in rt properties)
            std::array<vk::DeviceSize, 4> shaderRecordParameterSize = {0, 0, 0, 0};

            auto shaderHandleStorageSize = shaderGroupHandleSizeAligned * m_shaderGroups.size();

            std::vector<std::uint8_t> shaderHandleStorage;
            shaderHandleStorage.resize(shaderHandleStorageSize, 0);

            m_shaderBindingTable = std::make_unique<vkfw_core::gfx::HostBuffer>(
                m_device, fmt::format("{}:ShaderBindingTable", GetName()),
                vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eHostVisible);

            m_device->GetHandle().getRayTracingShaderGroupHandlesKHR(GetHandle(), 0,
                                                                     static_cast<std::uint32_t>(m_shaderGroups.size()),
                                                                     vk::ArrayProxy<std::uint8_t>(shaderHandleStorage));

            {
                std::size_t shaderBindingTableTotalSize = 0;
                std::array<std::vector<std::uint8_t>, 4> shaderBindingTables;
                for (std::size_t i_type = 0; i_type < m_shaderGroupIndexesByType.size(); ++i_type) {

                    m_shaderGroupTypeEntrySize[i_type] = m_device->CalculateSBTHandleAlignment(shaderGroupHandleSize
                                                                                  + shaderRecordParameterSize[i_type]);
                    shaderBindingTables[i_type].resize(
                        m_shaderGroupTypeEntrySize[i_type] * m_shaderGroupIndexesByType[i_type].size(), 0);
                    auto* data = shaderBindingTables[i_type].data();
                    m_shaderGroupTypeOffset[i_type] = shaderBindingTableTotalSize;
                    shaderBindingTableTotalSize += m_device->CalculateSBTBufferAlignment(shaderBindingTables[i_type].size());

                    for (std::size_t i_group = 0; i_group < m_shaderGroupIndexesByType[i_type].size(); ++i_group) {
                        memcpy(data,
                               shaderHandleStorage.data()
                                   + m_shaderGroupIndexesByType[i_type][i_group] * shaderGroupHandleSizeAligned,
                               shaderGroupHandleSize);
                        // TODO: copy shader record parameters. [10/3/2021 Sebastian Maisch]
                        data += m_shaderGroupTypeEntrySize[i_type];
                    }
                }

                std::vector<std::uint8_t> shaderBindingTableTotal;
                shaderBindingTableTotal.resize(shaderBindingTableTotalSize, 0);
                for (std::size_t i_type = 0; i_type < m_shaderGroupIndexesByType.size(); ++i_type) {
                    memcpy(shaderBindingTableTotal.data() + m_shaderGroupTypeOffset[i_type],
                           shaderBindingTables[i_type].data(), shaderBindingTables[i_type].size());
                }

                m_shaderBindingTable->InitializeData(shaderBindingTableTotal);
                // TODO: might be (slightly) faster to have a device memory SBT. [10/1/2021 Sebastian Maisch]
            }
        }

        auto sbtDeviceAddress = m_shaderBindingTable->GetDeviceAddress().deviceAddress;
        if (m_shaderGroupIndexesByType[0].size() != 1) {
            spdlog::error("Only a single ray generation shader is allowed per pipeline.");
            throw std::runtime_error("Only a single ray generation shader is allowed per pipeline.");
        }

        for (std::size_t i_type = 0; i_type < m_shaderGroupIndexesByType.size(); ++i_type) {
            auto deviceAddress = sbtDeviceAddress + m_shaderGroupTypeOffset[i_type];
            auto elementStride = m_shaderGroupTypeEntrySize[i_type];
            m_sbtDeviceAddressRegions[i_type] = vk::StridedDeviceAddressRegionKHR{
                deviceAddress, elementStride, m_shaderGroupIndexesByType[i_type].size() * elementStride};
        }
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
