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
#include "gfx/vk/GraphicsPipeline.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/Texture2D.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vkfw_core::gfx {

    LogicalDevice::LogicalDevice(const cfg::WindowCfg& windowCfg, const vk::PhysicalDevice& phDevice,
                                 std::vector<DeviceQueueDesc> queueDescs,
                                 const std::vector<std::string>& requiredDeviceExtensions,
                                 const vk::SurfaceKHR& surface)
        :
        m_windowCfg(windowCfg),
        m_vkPhysicalDevice(phDevice),
        m_vkPhysicalDeviceLimits(phDevice.getProperties().limits), // NOLINT
        m_queueDescriptions(std::move(queueDescs))
    {
        std::map<std::uint32_t, std::vector<std::pair<std::uint32_t, std::uint32_t>>> deviceQFamilyToRequested;
        std::map<std::uint32_t, std::vector<float>> deviceQFamilyPriorities;
        for (auto i = 0U; i < m_queueDescriptions.size(); ++i) {
            for (auto j = 0U; j < m_queueDescriptions[i].m_priorities.size(); ++j) {
                deviceQFamilyToRequested[m_queueDescriptions[i].m_familyIndex].emplace_back(std::make_pair(i, j));
                deviceQFamilyPriorities[m_queueDescriptions[i].m_familyIndex].push_back(m_queueDescriptions[i].m_priorities[j]);
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

        auto deviceFeatures = m_vkPhysicalDevice.getFeatures();
        std::vector<const char*> enabledDeviceExtensions;

        {
            spdlog::info("VK Device Extensions:");
            auto extensions = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();
            for (const auto& extension : extensions) {
                spdlog::info("- {}[SpecVersion: {}]", extension.extensionName, extension.specVersion);
            }

            enabledDeviceExtensions.resize(requiredDeviceExtensions.size());
            // NOLINTNEXTLINE
            auto i = 0ULL;
            for (; i < requiredDeviceExtensions.size(); ++i) {
                enabledDeviceExtensions[i] = requiredDeviceExtensions[i].c_str();
            }

            auto dbgMkFound = std::find_if(extensions.begin(), extensions.end(),
                [](const vk::ExtensionProperties& extProps) { return std::strcmp(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, &extProps.extensionName[0]) == 0; });
            if (dbgMkFound != extensions.end()) {
                m_enableDebugMarkers = true;
                enabledDeviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            }
            if (surface) {
                enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            } // checked this extension earlier
        }

        vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), static_cast<std::uint32_t>(queueCreateInfo.size()), queueCreateInfo.data(),
            static_cast<std::uint32_t>(ApplicationBase::instance().GetVKValidationLayers().size()), ApplicationBase::instance().GetVKValidationLayers().data(),
            static_cast<std::uint32_t>(enabledDeviceExtensions.size()), enabledDeviceExtensions.data(),
            &deviceFeatures };

        m_vkDevice = m_vkPhysicalDevice.createDeviceUnique(deviceCreateInfo);
        m_vkQueuesByRequestedFamily.resize(m_queueDescriptions.size());
        for (auto i = 0U; i < m_queueDescriptions.size(); ++i) {
            m_vkQueuesByRequestedFamily[i].resize(m_queueDescriptions[i].m_priorities.size());
        }
        m_vkCmdPoolsByRequestedQFamily.resize(m_queueDescriptions.size());

        for (const auto& deviceQueueDesc : deviceQFamilyToRequested) {
            const auto& priorities = deviceQFamilyPriorities[deviceQueueDesc.first];
            const auto& mappings = deviceQueueDesc.second;
            m_vkQueuesByDeviceFamily[deviceQueueDesc.first].resize(priorities.size());

            if constexpr (use_debug_pipeline) {
                vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlags(),
                                                   m_singleQueueOnly ? 0 : deviceQueueDesc.first};
                m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first] = m_vkDevice->createCommandPoolUnique(poolInfo);
            } else {
                vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlags(), deviceQueueDesc.first};
                m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first] = m_vkDevice->createCommandPoolUnique(poolInfo);
            }

            vk::Queue vkSingleQueue;
            if constexpr (use_debug_pipeline) {
                if (m_singleQueueOnly) { vkSingleQueue = m_vkDevice->getQueue(0, 0); }
            }

            for (auto j = 0U; j < priorities.size(); ++j) {
                if constexpr (use_debug_pipeline) {
                    if (m_singleQueueOnly) {
                        m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] = vkSingleQueue;
                    } else {
                        m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] =
                            m_vkDevice->getQueue(deviceQueueDesc.first, j);
                    }
                } else {
                    m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j] = m_vkDevice->getQueue(deviceQueueDesc.first, j);
                }
                m_vkQueuesByRequestedFamily[mappings[j].first][mappings[j].second] = m_vkQueuesByDeviceFamily[deviceQueueDesc.first][j];
                m_vkCmdPoolsByRequestedQFamily[mappings[j].first] = *m_vkCmdPoolsByDeviceQFamily[deviceQueueDesc.first];
            }
        }

        if (m_enableDebugMarkers) {
            fpDebugMarkerSetObjectTagEXT =
                reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(LoadVKDeviceFunction( // NOLINT
                    "vkDebugMarkerSetObjectTagEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpDebugMarkerSetObjectNameEXT =
                reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(LoadVKDeviceFunction( // NOLINT
                    "vkDebugMarkerSetObjectNameEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerBeginEXT = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>( // NOLINT
                LoadVKDeviceFunction("vkCmdDebugMarkerBeginEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerEndEXT = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>( // NOLINT
                LoadVKDeviceFunction("vkCmdDebugMarkerEndEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerInsertEXT = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>( // NOLINT
                LoadVKDeviceFunction("vkCmdDebugMarkerInsertEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
        }

        m_shaderManager = std::make_unique<ShaderManager>(this);
        m_textureManager = std::make_unique<TextureManager>(this);

        m_dummyMemGroup = std::make_unique<MemoryGroup>(this, vk::MemoryPropertyFlags());
        m_dummyTexture = m_textureManager->GetResource("dummy.png", true, true, *m_dummyMemGroup); // , std::vector<std::uint32_t>{ {0, 1} }

        QueuedDeviceTransfer transfer{ this, std::make_pair(0, 0) };
        m_dummyMemGroup->FinalizeDeviceGroup();
        m_dummyMemGroup->TransferData(transfer);
        transfer.FinishTransfer();
    }


    LogicalDevice::~LogicalDevice()
    {
        m_vkCmdPoolsByDeviceQFamily.clear();
        m_vkCmdPoolsByRequestedQFamily.clear();
        m_vkQueuesByDeviceFamily.clear();
        m_vkQueuesByRequestedFamily.clear();
    }

    PFN_vkVoidFunction LogicalDevice::LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory) const
    {
        auto func = vkGetDeviceProcAddr(static_cast<vk::Device>(*m_vkDevice), functionName.c_str());
        if (func == nullptr) {
            if (mandatory) {
                spdlog::critical("Could not load device function '{}' [{}].",
                                 functionName, extensionName);
                throw std::runtime_error("Could not load mandatory device function.");
            }
            spdlog::warn("Could not load device function '{}' [{}].", functionName, extensionName);
        }

        return func;
    }

    vk::UniqueCommandPool LogicalDevice::CreateCommandPoolForQueue(unsigned int familyIndex, const vk::CommandPoolCreateFlags& flags) const
    {
        if constexpr (use_debug_pipeline) {
            vk::CommandPoolCreateInfo poolInfo{flags, m_singleQueueOnly ? 0 : familyIndex};
            return m_vkDevice->createCommandPoolUnique(poolInfo);
        } else {
            vk::CommandPoolCreateInfo poolInfo{flags, familyIndex};
            return m_vkDevice->createCommandPoolUnique(poolInfo);
        }
    }

    std::unique_ptr<GraphicsPipeline> LogicalDevice::CreateGraphicsPipeline(const std::vector<std::string>& shaderNames,
        const glm::uvec2& size, unsigned int numBlendAttachments)
    {
        std::vector<std::shared_ptr<Shader>> shaders(shaderNames.size());
        for (auto i = 0U; i < shaderNames.size(); ++i) { shaders[i] = m_shaderManager->GetResource(shaderNames[i]); }

        return std::make_unique<GraphicsPipeline>(this, shaders, size, numBlendAttachments);
    }

    VkResult LogicalDevice::DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT* tagInfo) const
    {
        return m_enableDebugMarkers ? fpDebugMarkerSetObjectTagEXT(device, tagInfo) : VK_SUCCESS;
    }

    VkResult LogicalDevice::DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT* nameInfo) const
    {
        return m_enableDebugMarkers ? fpDebugMarkerSetObjectNameEXT(device, nameInfo) : VK_SUCCESS;
    }

    void LogicalDevice::CmdDebugMarkerBeginEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const
    {
        if (m_enableDebugMarkers) { fpCmdDebugMarkerBeginEXT(cmdBuffer, markerInfo); }
    }

    void LogicalDevice::CmdDebugMarkerEndEXT(VkCommandBuffer cmdBuffer) const
    {
        if (m_enableDebugMarkers) { fpCmdDebugMarkerEndEXT(cmdBuffer); }
    }

    void LogicalDevice::CmdDebugMarkerInsertEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const
    {
        if (m_enableDebugMarkers) { fpCmdDebugMarkerInsertEXT(cmdBuffer, markerInfo); }
    }

    constexpr std::size_t CalcAlignedSize(std::size_t size, std::size_t alignment)
    {
        return size + alignment - 1 - ((size + alignment - 1) % alignment);
    }

    std::size_t LogicalDevice::CalculateUniformBufferAlignment(std::size_t size) const
    {
        auto factor = m_vkPhysicalDeviceLimits.minUniformBufferOffsetAlignment;
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
}
