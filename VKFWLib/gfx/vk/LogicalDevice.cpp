/**
 * @file   LogicalDevice.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Implementation of the LogicalDevice class.
 */

#include "LogicalDevice.h"
#include <app/ApplicationBase.h>

namespace vku { namespace gfx {

    LogicalDevice::LogicalDevice(const vk::PhysicalDevice& phDevice, const std::vector<DeviceQueueDesc>& queueDescs, const vk::SurfaceKHR& surface) :
        vkPhysicalDevice_(phDevice)
    {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
        for (const auto& queueDesc : queueDescs) {
            queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), queueDesc.familyIndex_, static_cast<uint32_t>(queueDesc.priorities_.size()), queueDesc.priorities_.data());
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
        vkQueues_.resize(queueDescs.size());
        for (auto i = 0U; i < queueDescs.size(); ++i) {
            vkQueues_[i].resize(queueDescs[i].priorities_.size());
            for (auto j = 0U; j < vkQueues_[i].size(); ++j) {
                vkQueues_[i][j] = vkDevice_.getQueue(queueDescs[i].familyIndex_, j);
            }
        }

        if (enableDebugMarkers_) {
            fpDebugMarkerSetObjectTagEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(LoadVKDeviceFunction("vkDebugMarkerSetObjectTagEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpDebugMarkerSetObjectNameEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(LoadVKDeviceFunction("vkDebugMarkerSetObjectNameEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerBeginEXT = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerBeginEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerEndEXT = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerEndEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
            fpCmdDebugMarkerInsertEXT = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(LoadVKDeviceFunction("vkCmdDebugMarkerInsertEXT", VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true));
        }
    }


    LogicalDevice::~LogicalDevice()
    {
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
}}
