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
                                           std::vector<RTShaderInfo>&& shaders)
        : VulkanObjectPrivateWrapper{device->GetHandle(), name, vk::UniquePipeline{}}
        , m_device{device}
        , m_shaders{shaders}
        , m_barrier{device}
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

    void RayTracingPipeline::BindPipeline(CommandBuffer& cmdBuffer)
    {
        m_barrier.Record(cmdBuffer);
        cmdBuffer.GetHandle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, GetHandle());
    }

    void RayTracingPipeline::ResetShaders(std::vector<RTShaderInfo>&& shaders)
    {
        m_shaders = std::move(shaders);
        m_shaderStages.resize(m_shaders.size());
        m_shaderGroups.clear();

        if (m_shaders.empty()) {
            return;
        }

        vk::RayTracingShaderGroupCreateInfoKHR rayGenShaderGroup{vk::RayTracingShaderGroupTypeKHR::eGeneral,
                                                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR};
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> missShaderGroups;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> hitShaderGroups;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> callableShaderGroups;

        auto ensureGroupsSize = [](std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& groups,
                                   std::uint32_t shaderGroup, vk::RayTracingShaderGroupTypeKHR groupType) {
            if (groups.size() <= shaderGroup) {
                groups.resize(shaderGroup + 1, vk::RayTracingShaderGroupCreateInfoKHR{
                                                   groupType, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
                                                   VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
            }
        };

        auto setGeneralShader = [](vk::RayTracingShaderGroupCreateInfoKHR& group, std::uint32_t groupShader,
                                   std::string_view errorMessage) {
            if (group.generalShader != VK_SHADER_UNUSED_KHR) {
                spdlog::error(errorMessage);
                throw std::runtime_error(errorMessage.data());
            }
            group.generalShader = groupShader;
        };

        for (std::size_t i = 0; i < m_shaders.size(); ++i) {
            m_shaders[i].shader->FillShaderStageInfo(m_shaderStages[i]);
            auto shaderStage = static_cast<std::uint32_t>(i);

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eRaygenKHR) {
                setGeneralShader(rayGenShaderGroup, shaderStage, "There can only be a single ray generation shader.");
            }

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eMissKHR) {
                ensureGroupsSize(missShaderGroups, m_shaders[i].shaderGroup,
                                 vk::RayTracingShaderGroupTypeKHR::eGeneral);
                setGeneralShader(missShaderGroups[m_shaders[i].shaderGroup], shaderStage,
                                 "Miss shader groups can not have more than one miss shader.");
            }

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eClosestHitKHR) {
                ensureGroupsSize(hitShaderGroups, m_shaders[i].shaderGroup,
                                 vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
                if (hitShaderGroups[m_shaders[i].shaderGroup].closestHitShader != VK_SHADER_UNUSED_KHR) {
                    std::string_view errorMessage = "There can only be a single closest hit shader per group.";
                    spdlog::error(errorMessage);
                    throw std::runtime_error(errorMessage.data());
                }
                hitShaderGroups[m_shaders[i].shaderGroup].closestHitShader = shaderStage;
            }

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eAnyHitKHR) {
                ensureGroupsSize(hitShaderGroups, m_shaders[i].shaderGroup,
                                 vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
                if (hitShaderGroups[m_shaders[i].shaderGroup].anyHitShader != VK_SHADER_UNUSED_KHR) {
                    std::string_view errorMessage = "There can only be a single any hit shader per group.";
                    spdlog::error(errorMessage);
                    throw std::runtime_error(errorMessage.data());
                }
                hitShaderGroups[m_shaders[i].shaderGroup].anyHitShader = shaderStage;
            }

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eIntersectionKHR) {
                ensureGroupsSize(hitShaderGroups, m_shaders[i].shaderGroup,
                                 vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
                if (hitShaderGroups[m_shaders[i].shaderGroup].intersectionShader != VK_SHADER_UNUSED_KHR) {
                    std::string_view errorMessage = "There can only be a single intersection shader per group.";
                    spdlog::error(errorMessage);
                    throw std::runtime_error(errorMessage.data());
                }
                hitShaderGroups[m_shaders[i].shaderGroup].intersectionShader = shaderStage;
                hitShaderGroups[m_shaders[i].shaderGroup].type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
            }

            if (m_shaderStages[i].stage == vk::ShaderStageFlagBits::eCallableKHR) {
                ensureGroupsSize(callableShaderGroups, m_shaders[i].shaderGroup,
                                 vk::RayTracingShaderGroupTypeKHR::eGeneral);
                setGeneralShader(callableShaderGroups[m_shaders[i].shaderGroup], shaderStage,
                                 "Callable shader groups can not have more than one miss shader.");
            }
        }

        for (auto& shaderGroupIndices : m_shaderGroupIndexesByType) { shaderGroupIndices.clear(); }

        auto addShaderGroup = [this](const vk::RayTracingShaderGroupCreateInfoKHR& shaderGroup,
                                 vk::ShaderStageFlagBits stage) {
            m_shaderGroupIndexesByType[ShaderTypeToTypeGroupIndex(stage)].push_back(static_cast<std::uint32_t>(m_shaderGroups.size()));
            ValidateShaderGroup(shaderGroup);
            m_shaderGroups.emplace_back(shaderGroup);
        };

        addShaderGroup(rayGenShaderGroup, vk::ShaderStageFlagBits::eRaygenKHR);
        for (const auto& missGroup : missShaderGroups) { addShaderGroup(missGroup, vk::ShaderStageFlagBits::eMissKHR); }
        for (const auto& hitGroup : hitShaderGroups) { addShaderGroup(hitGroup, vk::ShaderStageFlagBits::eClosestHitKHR); }
        for (const auto& callableGroup : callableShaderGroups) {
            addShaderGroup(callableGroup, vk::ShaderStageFlagBits::eCallableKHR);
        }
    }

    void RayTracingPipeline::InitializeShaderBindingTable()
    {
        m_barrier = PipelineBarrier{m_device};
        auto shaderGroupHandleSize = m_device->GetDeviceRayTracingPipelineProperties().shaderGroupHandleSize;
        auto shaderGroupHandleSizeAligned = m_device->CalculateSBTHandleAlignment(shaderGroupHandleSize);
        {
            // TODO: allow shader record parameters. [10/1/2021 Sebastian Maisch]
            // Parameters are memory after the shader group handle (see shaderGroupHandleSize in rt properties)
            // that can contain any variables that can fit into a uniform buffer.
            // max amount is maxShaderGroupStride - shaderGroupHandleSize (see both in rt properties)
            std::array<vk::DeviceSize, 4> shaderRecordParameterSize = {0, 0, 0, 0};

            auto shaderHandleStorageSize = shaderGroupHandleSizeAligned * m_shaderGroups.size();

            m_shaderBindingTable = std::make_unique<vkfw_core::gfx::HostBuffer>(
                m_device, fmt::format("{}:ShaderBindingTable", GetName()),
                vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eHostVisible);

            std::vector<std::uint8_t> shaderHandleStorage;
            shaderHandleStorage = m_device->GetHandle().getRayTracingShaderGroupHandlesKHR<std::uint8_t>(
                GetHandle(), 0, static_cast<std::uint32_t>(m_shaderGroups.size()), shaderHandleStorageSize);

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

        auto sbtDeviceAddress = m_shaderBindingTable
                                    ->GetDeviceAddress(vk::AccessFlagBits2KHR::eShaderRead,
                                                       vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR, m_barrier)
                                    .deviceAddress;
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
