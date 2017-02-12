/**
 * @file   LogicalDevice.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Implementation of the LogicalDevice class.
 */

#include "LogicalDevice.h"
#include <app/ApplicationBase.h>
#include "Shader.h"
#include "core/resources/ShaderManager.h"
#include "GraphicsPipeline.h"

namespace vku { namespace gfx {

    LogicalDevice::LogicalDevice(const vk::PhysicalDevice& phDevice, const std::vector<DeviceQueueDesc>& queueDescs, const vk::SurfaceKHR& surface) :
        vkPhysicalDevice_(phDevice),
        vkPhysicalDeviceLimits_(phDevice.getProperties().limits),
        queueDescriptions_(queueDescs)
    {
        std::map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> deviceQFamilyToRequested;
        std::map<uint32_t, std::vector<float>> deviceQFamilyPriorities;
        for (auto i = 0U; i < queueDescriptions_.size(); ++i) {
            for (auto j = 0U; j < queueDescriptions_[i].priorities_.size(); ++j) {
                deviceQFamilyToRequested[queueDescriptions_[i].familyIndex_].emplace_back(std::make_pair(i, j));
                deviceQFamilyPriorities[queueDescriptions_[i].familyIndex_].push_back(queueDescriptions_[i].priorities_[j]);
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
        for (const auto& queueDesc : queueDescriptions_) {
            auto& priorities = deviceQFamilyPriorities[queueDesc.familyIndex_];
            queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), queueDesc.familyIndex_, static_cast<uint32_t>(priorities.size()), priorities.data());
        }
        auto deviceFeatures = vkPhysicalDevice_.getFeatures();
        std::vector<const char*> enabledDeviceExtensions;

        {
            LOG(INFO) << "VK Device Extensions:";
            auto extensions = vkPhysicalDevice_.enumerateDeviceExtensionProperties();
            for (const auto& extension : extensions) LOG(INFO) << "- " << extension.extensionName << "[SpecVersion:" << extension.specVersion << "]";

            auto dbgMkFound = std::find_if(extensions.begin(), extensions.end(),
                [](const vk::ExtensionProperties& extProps) { return std::strcmp(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, extProps.extensionName) == 0; });
            if (dbgMkFound != extensions.end()) {
                enableDebugMarkers_ = true;
                enabledDeviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            }
            if (surface) enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); // checked this extension earlier
        }

        vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfo.size()), queueCreateInfo.data(),
            static_cast<uint32_t>(ApplicationBase::instance().GetVKValidationLayers().size()), ApplicationBase::instance().GetVKValidationLayers().data(),
            static_cast<uint32_t>(enabledDeviceExtensions.size()), enabledDeviceExtensions.data(),
            &deviceFeatures };

        vkDevice_ = vkPhysicalDevice_.createDevice(deviceCreateInfo);
        vkQueuesByRequestedFamily_.resize(queueDescriptions_.size());
        for (auto i = 0U; i < queueDescriptions_.size(); ++i) vkQueuesByRequestedFamily_[i].resize(queueDescriptions_[i].priorities_.size());
        vkCmdPoolsByRequestedQFamily_.resize(queueDescriptions_.size());

        for (const auto& deviceQueueDesc : deviceQFamilyToRequested) {
            const auto& priorities = deviceQFamilyPriorities[deviceQueueDesc.first];
            const auto& mappings = deviceQueueDesc.second;
            vkQueuesByDeviceFamily_[deviceQueueDesc.first].resize(priorities.size());

            vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlags(), deviceQueueDesc.first };
            vkCmdPoolsByDeviceQFamily_[deviceQueueDesc.first] = vkDevice_.createCommandPool(poolInfo);

            for (auto j = 0U; j < priorities.size(); ++j) {
                vkQueuesByDeviceFamily_[deviceQueueDesc.first][j] = vkDevice_.getQueue(deviceQueueDesc.first, j);
                vkQueuesByRequestedFamily_[mappings[j].first][mappings[j].second] = vkQueuesByDeviceFamily_[deviceQueueDesc.first][j];
                vkCmdPoolsByRequestedQFamily_[mappings[j].first] = vkCmdPoolsByDeviceQFamily_[deviceQueueDesc.first];
            }
        }

        if (enableDebugMarkers_) {
            fpDebugMarkerSetObjectTagEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(LoadVKDeviceFunction("vkDebugMarkerSetObjectTagEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpDebugMarkerSetObjectNameEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(LoadVKDeviceFunction("vkDebugMarkerSetObjectNameEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerBeginEXT = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerBeginEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerEndEXT = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerEndEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerInsertEXT = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerInsertEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
        }

        shaderManager_ = std::make_unique<ShaderManager>(this);
    }


    LogicalDevice::~LogicalDevice()
    {
        for (auto& cmdPool : vkCmdPoolsByDeviceQFamily_) {
            if (cmdPool.second) vkDevice_.destroyCommandPool(cmdPool.second);
        }
        vkCmdPoolsByDeviceQFamily_.clear();
        vkCmdPoolsByRequestedQFamily_.clear();
        vkQueuesByDeviceFamily_.clear();
        vkQueuesByRequestedFamily_.clear();

        if (vkDevice_) vkDevice_.destroy();
    }

    PFN_vkVoidFunction LogicalDevice::LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory) const
    {
        auto func = vkGetDeviceProcAddr(static_cast<vk::Device>(vkDevice_), functionName.c_str());
        if (func == nullptr) {
            if (mandatory) {
                LOG(FATAL) << "Could not load device function '" << functionName << "' [" << extensionName << "].";
                throw std::runtime_error("Could not load mandatory device function.");
            }
            LOG(WARNING) << "Could not load device function '" << functionName << "' [" << extensionName << "].";
        }

        return func;
    }

    std::unique_ptr<GraphicsPipeline> LogicalDevice::CreateGraphicsPipeline(const std::vector<std::string>& shaderNames, const glm::uvec2& size, unsigned numBlendAttachments)
    {
        std::vector<std::shared_ptr<Shader>> shaders(shaderNames.size());
        for (auto i = 0U; i < shaderNames.size(); ++i) shaders[i] = shaderManager_->GetResource(shaderNames[i]);

        return std::make_unique<GraphicsPipeline>(this, shaders, size, numBlendAttachments);
    }

    VkResult LogicalDevice::DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT* tagInfo) const
    {
        return enableDebugMarkers_ ? fpDebugMarkerSetObjectTagEXT(device, tagInfo) : VK_SUCCESS;
    }

    VkResult LogicalDevice::DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT* nameInfo) const
    {
        return enableDebugMarkers_ ? fpDebugMarkerSetObjectNameEXT(device, nameInfo) : VK_SUCCESS;
    }

    void LogicalDevice::CmdDebugMarkerBeginEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const
    {
        if (enableDebugMarkers_) fpCmdDebugMarkerBeginEXT(cmdBuffer, markerInfo);
    }

    void LogicalDevice::CmdDebugMarkerEndEXT(VkCommandBuffer cmdBuffer) const
    {
        if (enableDebugMarkers_) fpCmdDebugMarkerEndEXT(cmdBuffer);
    }

    void LogicalDevice::CmdDebugMarkerInsertEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const
    {
        if (enableDebugMarkers_) fpCmdDebugMarkerInsertEXT(cmdBuffer, markerInfo);
    }

    size_t LogicalDevice::CalculateUniformBufferAlignment(size_t size) const
    {
        auto factor = vkPhysicalDeviceLimits_.minUniformBufferOffsetAlignment;
        return size + factor - 1 - ((size + factor - 1) % factor);
    }
}}
