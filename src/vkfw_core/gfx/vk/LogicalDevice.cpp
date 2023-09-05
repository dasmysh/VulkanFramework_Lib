/**
 * @file   LogicalDevice.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Implementation of the LogicalDevice class.
 */

#include "gfx/vk/LogicalDevice.h"
#include <app/ApplicationBase.h>
#include "gfx/vk/Shader.h"
#include "core/resources/ShaderManager.h"
#include "gfx/vk/pipeline/GraphicsPipeline.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/Texture2D.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vkfw_core::gfx {

    LogicalDevice::LogicalDevice(const cfg::WindowCfg& windowCfg, vk::PhysicalDevice phDevice,
                                 std::vector<DeviceQueueDesc> queueDescs,
                                 const std::vector<std::string>& requiredDeviceExtensions, void* featuresNextChain,
                                 const Surface& surface)
        : VulkanObjectWrapper{nullptr, fmt::format("WindowDevice-{}", windowCfg.m_windowTitle), vk::UniqueDevice{}}
        , m_windowCfg{windowCfg}
        , m_vkPhysicalDevice{phDevice}
        , m_vkPhysicalDeviceLimits{phDevice.getProperties().limits} // NOLINT
        , m_queueDescriptions{std::move(queueDescs)}
        , m_resourceReleaser{std::make_unique<ResourceReleaser>(this)}
    {
        std::map<std::uint32_t, std::vector<std::pair<std::uint32_t, std::uint32_t>>> deviceQFamilyToRequested;
        std::map<std::uint32_t, std::vector<float>> deviceQFamilyPriorities;
        for (auto i = 0U; i < m_queueDescriptions.size(); ++i) {
            for (auto j = 0U; j < m_queueDescriptions[i].m_priorities.size(); ++j) {
                deviceQFamilyToRequested[m_queueDescriptions[i].m_familyIndex].emplace_back(std::make_pair(i, j));
                deviceQFamilyPriorities[m_queueDescriptions[i].m_familyIndex].push_back(
                    m_queueDescriptions[i].m_priorities[j]);
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
        if constexpr (use_debug_pipeline) {
            {
                auto devProps = m_vkPhysicalDevice.getProperties();
                if (devProps.pipelineCacheUUID[0] == 'r' && devProps.pipelineCacheUUID[1] == 'd'
                    && devProps.pipelineCacheUUID[2] == 'o' && devProps.pipelineCacheUUID[3] == 'c') {
                    m_singleQueueOnly = true;
                }
            }

            if (m_singleQueueOnly) {
                float prio = 1.0f;
                queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), 0, 1, &prio);
            } else {
                for (const auto& queueDesc : m_queueDescriptions) {
                    auto& priorities = deviceQFamilyPriorities[queueDesc.m_familyIndex];
                    queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), queueDesc.m_familyIndex,
                                                 static_cast<std::uint32_t>(priorities.size()), priorities.data());
                }
            }
        } else {
            for (const auto& queueDesc : m_queueDescriptions) {
                auto& priorities = deviceQFamilyPriorities[queueDesc.m_familyIndex];
                queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), queueDesc.m_familyIndex,
                                             static_cast<std::uint32_t>(priorities.size()), priorities.data());
            }
        }

        m_deviceFeatures = m_vkPhysicalDevice.getFeatures();
        m_deviceProperties = m_vkPhysicalDevice.getProperties();
        std::vector<const char*> enabledDeviceExtensions;

        if (m_windowCfg.m_useRayTracing) {
            auto features =
                m_vkPhysicalDevice
                    .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                                  vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
            m_raytracingPipelineFeatures = features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
            m_accelerationStructureFeatures = features.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
            auto properties =
                m_vkPhysicalDevice
                    .getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
                                    vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
            m_raytracingPipelineProperties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
            m_accelerationStructureProperties = properties.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
        }

        {
            auto extensions = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();
            if constexpr (verbose_feature_logging) {
                spdlog::info("VK Device Extensions:");
                for (const auto& extension : extensions) {
                    spdlog::info("- {}[SpecVersion: {}]", extension.extensionName.data(), extension.specVersion);
                }
            }

            enabledDeviceExtensions.resize(requiredDeviceExtensions.size());
            // NOLINTNEXTLINE
            auto i = 0ULL;
            for (; i < requiredDeviceExtensions.size(); ++i) {
                enabledDeviceExtensions[i] = requiredDeviceExtensions[i].c_str();
            }

            if (surface) {
                enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            } // checked this extension earlier
        }

        vk::DeviceCreateInfo deviceCreateInfo{
            vk::DeviceCreateFlags(),
            static_cast<std::uint32_t>(queueCreateInfo.size()),
            queueCreateInfo.data(),
            static_cast<std::uint32_t>(ApplicationBase::instance().GetVKValidationLayers().size()),
            ApplicationBase::instance().GetVKValidationLayers().data(),
            static_cast<std::uint32_t>(enabledDeviceExtensions.size()),
            enabledDeviceExtensions.data(),
            &m_deviceFeatures};
        vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
        if (featuresNextChain) {
            physicalDeviceFeatures2.features = m_deviceFeatures;
            physicalDeviceFeatures2.pNext = featuresNextChain;
            deviceCreateInfo.pEnabledFeatures = nullptr;
            deviceCreateInfo.pNext = &physicalDeviceFeatures2;
        }

        {
            auto vkUDevice = m_vkPhysicalDevice.createDeviceUnique(deviceCreateInfo);
            auto vkDevice = *vkUDevice;
            SetHandle(vkDevice, std::move(vkUDevice));
        }

        m_queuesByRequestedFamily.resize(m_queueDescriptions.size());
        for (auto i = 0U; i < m_queueDescriptions.size(); ++i) {
            m_queuesByRequestedFamily[i].resize(m_queueDescriptions[i].m_priorities.size(), Queue{});
        }
        m_cmdPoolsByRequestedQFamily.resize(m_queueDescriptions.size(), nullptr);

        for (const auto& deviceQueueDesc : deviceQFamilyToRequested) {
            const auto& priorities = deviceQFamilyPriorities[deviceQueueDesc.first];
            const auto& mappings = deviceQueueDesc.second;
            m_vkQueuesByDeviceFamily[deviceQueueDesc.first].resize(priorities.size());

            if constexpr (use_debug_pipeline) {
                vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlags(),
                                                   m_singleQueueOnly ? 0 : deviceQueueDesc.first};
                m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first] = CommandPool{
                    GetHandle(), fmt::format("Dev-{} QueueCommandPool{}", windowCfg.m_windowTitle, mappings[0].first),
                    mappings[0].first, GetHandle().createCommandPoolUnique(poolInfo)};
            } else {
                vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlags(), deviceQueueDesc.first};
                m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first] = CommandPool{
                    GetHandle(), fmt::format("Dev-{} QueueCommandPool{}", windowCfg.m_windowTitle, mappings[0].first),
                    mappings[0].first, GetHandle().createCommandPoolUnique(poolInfo)};
            }

            vk::Queue vkSingleQueue;
            if constexpr (use_debug_pipeline) {
                if (m_singleQueueOnly) { vkSingleQueue = GetHandle().getQueue(0, 0); }
            }

            for (auto j = 0U; j < priorities.size(); ++j) {
                if constexpr (use_debug_pipeline) {
                    if (m_singleQueueOnly) {
                        m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] = vkSingleQueue;
                    } else {
                        m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] =
                            GetHandle().getQueue(deviceQueueDesc.first, j);
                    }
                } else {
                    m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] = GetHandle().getQueue(deviceQueueDesc.first, j);
                }
                m_cmdPoolsByRequestedQFamily[mappings[j].first] = &m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first];
                m_queuesByRequestedFamily[mappings[j].first][mappings[j].second] =
                    Queue{GetHandle(), fmt::format("Dev-{} Queue{}-{}", windowCfg.m_windowTitle, mappings[j].first, mappings[j].second),
                          m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j],
                          *m_cmdPoolsByRequestedQFamily[mappings[j].first]};
            }
        }

        m_shaderManager = std::make_unique<ShaderManager>(this);
        m_textureManager = std::make_unique<TextureManager>(this);

        m_dummyMemGroup = std::make_unique<MemoryGroup>(this, "DummyMemGroup", vk::MemoryPropertyFlags());
        m_dummyTexture = m_textureManager->GetResource("dummy.png", true, true,
                                                       *m_dummyMemGroup); // , std::vector<std::uint32_t>{ {0, 1} }

        QueuedDeviceTransfer transfer{this, GetQueue(0, 0)};
        m_dummyMemGroup->FinalizeDeviceGroup();
        m_dummyMemGroup->TransferData(transfer);
        transfer.FinishTransfer();

        {
            auto cmdBuffer = CommandBuffer::beginSingleTimeSubmit(this, "TransferImageLayoutsInitialCommandBuffer",
                                                                  "TransferImageLayoutsInitial", GetCommandPool(0));
            PipelineBarrier barrier{this};
            m_dummyTexture->GetTexture().AccessBarrier(vk::AccessFlagBits2KHR::eShaderRead,
                                                       vk::PipelineStageFlagBits2KHR::eFragmentShader,
                                                       vk::ImageLayout::eShaderReadOnlyOptimal, barrier);
            barrier.Record(cmdBuffer);
            auto fence = CommandBuffer::endSingleTimeSubmit(GetQueue(0, 0), cmdBuffer, {}, {});
            if (auto r = GetHandle().waitForFences({fence->GetHandle()}, VK_TRUE, vkfw_core::defaultFenceTimeout);
                r != vk::Result::eSuccess) {
                spdlog::error("Could not wait for fence while transitioning layout: {}.", vk::to_string(r));
                throw std::runtime_error("Could not wait for fence while transitioning layout.");
            }
        }
    }


    LogicalDevice::~LogicalDevice()
    {
        m_vkCmdPoolsByDeviceQFamily.clear();
        m_cmdPoolsByRequestedQFamily.clear();
        m_vkQueuesByDeviceFamily.clear();
        m_queuesByRequestedFamily.clear();
    }

    CommandPool LogicalDevice::CreateCommandPoolForQueue(std::string_view name, unsigned int familyIndex, const vk::CommandPoolCreateFlags& flags) const
    {
        if constexpr (use_debug_pipeline) {
            vk::CommandPoolCreateInfo poolInfo{flags, m_singleQueueOnly ? 0 : m_queueDescriptions[familyIndex].m_familyIndex};
            return CommandPool{GetHandle(), name, familyIndex, GetHandle().createCommandPoolUnique(poolInfo)};
        } else {
            vk::CommandPoolCreateInfo poolInfo{flags, familyIndex};
            return CommandPool{GetHandle(), name, familyIndex, GetHandle().createCommandPoolUnique(poolInfo)};
        }
    }

    std::unique_ptr<GraphicsPipeline> LogicalDevice::CreateGraphicsPipeline(const std::vector<std::string>& shaderNames,
        const glm::uvec2& size, unsigned int numBlendAttachments)
    {
        std::vector<std::shared_ptr<Shader>> shaders(shaderNames.size());
        for (auto i = 0U; i < shaderNames.size(); ++i) { shaders[i] = m_shaderManager->GetResource(shaderNames[i]); }

        return std::make_unique<GraphicsPipeline>(this, fmt::format("{}", fmt::join(shaderNames, "|")), std::move(shaders), size,
                                                  numBlendAttachments);
    }

    constexpr std::size_t CalcAlignedSize(std::size_t size, std::size_t alignment)
    {
        // return size + alignment - 1 - ((size + alignment - 1) % alignment);
        // return (size + alignment - 1) & ~(alignment - 1);
        return alignment * ((size + alignment - 1) / alignment);
    }

    std::size_t LogicalDevice::CalculateUniformBufferAlignment(std::size_t size) const
    {
        auto factor = m_vkPhysicalDeviceLimits.minUniformBufferOffsetAlignment;
        return CalcAlignedSize(size, factor);
    }

    std::size_t LogicalDevice::CalculateStorageBufferAlignment(std::size_t size) const
    {
        auto factor = m_vkPhysicalDeviceLimits.minStorageBufferOffsetAlignment;
        return CalcAlignedSize(size, factor);
    }

    std::size_t LogicalDevice::CalculateASScratchBufferBufferAlignment(std::size_t size) const
    {
        auto factor = m_accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
        return CalcAlignedSize(size, factor);
    }

    std::size_t LogicalDevice::CalculateBufferImageOffset(const Texture& second, std::size_t currentOffset) const
    {
        if (second.GetDescriptor().m_imageTiling == vk::ImageTiling::eOptimal) {
            return CalcAlignedSize(currentOffset, m_vkPhysicalDeviceLimits.bufferImageGranularity);
        }
        return currentOffset;
    }
    std::size_t LogicalDevice::CalculateImageImageOffset(const Texture & first, const Texture & second, std::size_t currentOffset) const
    {
        if (first.GetDescriptor().m_imageTiling != second.GetDescriptor().m_imageTiling) {
            return CalcAlignedSize(currentOffset, m_vkPhysicalDeviceLimits.bufferImageGranularity);
        }
        return currentOffset;
    }
    std::pair<unsigned int, vk::Format> LogicalDevice::FindSupportedFormat(const std::vector<std::pair<unsigned int, vk::Format>>& candidates,
        vk::ImageTiling tiling, const vk::FormatFeatureFlags& features) const
    {
        for (const auto& cand : candidates) {
            vk::FormatProperties formatProps = m_vkPhysicalDevice.getFormatProperties(cand.second);

            if ((tiling == vk::ImageTiling::eLinear && (formatProps.linearTilingFeatures & features) == features)
                || (tiling == vk::ImageTiling::eOptimal
                    && (formatProps.optimalTilingFeatures & features) == features)) {
                return cand;
            }
        }

        throw std::runtime_error("No candidate format supported.");
    }

    std::size_t LogicalDevice::CalculateSBTBufferAlignment(std::size_t size) const
    {
        auto factor = m_raytracingPipelineProperties.shaderGroupBaseAlignment;
        return CalcAlignedSize(size, factor);
    }

    std::size_t LogicalDevice::CalculateSBTHandleAlignment(std::size_t size) const
    {
        auto factor = m_raytracingPipelineProperties.shaderGroupHandleAlignment;
        return CalcAlignedSize(size, factor);
    }

}
