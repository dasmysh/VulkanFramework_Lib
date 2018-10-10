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
#include "textures/Texture.h"

namespace vku::gfx {

    LogicalDevice::LogicalDevice(const cfg::WindowCfg& windowCfg, const vk::PhysicalDevice& phDevice,
        const std::vector<DeviceQueueDesc>& queueDescs, const vk::SurfaceKHR& surface) :
        windowCfg_(windowCfg),
        vkPhysicalDevice_(phDevice),
        vkPhysicalDeviceLimits_(phDevice.getProperties().limits),
        queueDescriptions_(queueDescs)
    {
        std::map<std::uint32_t, std::vector<std::pair<std::uint32_t, std::uint32_t>>> deviceQFamilyToRequested;
        std::map<std::uint32_t, std::vector<float>> deviceQFamilyPriorities;
        for (auto i = 0U; i < queueDescriptions_.size(); ++i) {
            for (auto j = 0U; j < queueDescriptions_[i].priorities_.size(); ++j) {
                deviceQFamilyToRequested[queueDescriptions_[i].familyIndex_].emplace_back(std::make_pair(i, j));
                deviceQFamilyPriorities[queueDescriptions_[i].familyIndex_].push_back(queueDescriptions_[i].priorities_[j]);
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
#ifdef FW_DEBUG_PIPELINE
        bool singleQueueOnly = false;
        {
            auto devProps = vkPhysicalDevice_.getProperties();
            if (devProps.pipelineCacheUUID[0] == 'r' && devProps.pipelineCacheUUID[1] == 'd' &&
                devProps.pipelineCacheUUID[2] == 'o' && devProps.pipelineCacheUUID[3] == 'c') singleQueueOnly = true;
        }

        if (singleQueueOnly) {
            float prio = 1.0f;
            queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), 0, 1, &prio);
        }
        else // i wonder if i can make this part even more unreadable ...
#endif
            for (const auto& queueDesc : queueDescriptions_) {
                auto& priorities = deviceQFamilyPriorities[queueDesc.familyIndex_];
                queueCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(), queueDesc.familyIndex_, static_cast<std::uint32_t>(priorities.size()), priorities.data());
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

        vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), static_cast<std::uint32_t>(queueCreateInfo.size()), queueCreateInfo.data(),
            static_cast<std::uint32_t>(ApplicationBase::instance().GetVKValidationLayers().size()), ApplicationBase::instance().GetVKValidationLayers().data(),
            static_cast<std::uint32_t>(enabledDeviceExtensions.size()), enabledDeviceExtensions.data(),
            &deviceFeatures };

        vkDevice_ = vkPhysicalDevice_.createDeviceUnique(deviceCreateInfo);
        vkQueuesByRequestedFamily_.resize(queueDescriptions_.size());
        for (auto i = 0U; i < queueDescriptions_.size(); ++i) vkQueuesByRequestedFamily_[i].resize(queueDescriptions_[i].priorities_.size());
        vkCmdPoolsByRequestedQFamily_.resize(queueDescriptions_.size());

        for (const auto& deviceQueueDesc : deviceQFamilyToRequested) {
            const auto& priorities = deviceQFamilyPriorities[deviceQueueDesc.first];
            const auto& mappings = deviceQueueDesc.second;
            vkQueuesByDeviceFamily_[deviceQueueDesc.first].resize(priorities.size());

            vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlags(), deviceQueueDesc.first };
            vkCmdPoolsByDeviceQFamily_[deviceQueueDesc.first] = vkDevice_->createCommandPoolUnique(poolInfo);

#ifdef FW_DEBUG_PIPELINE
            vk::Queue vkSingleQueue;
            if (singleQueueOnly) vkSingleQueue = vkDevice_->getQueue(0, 0);
#endif
            for (auto j = 0U; j < priorities.size(); ++j) {
#ifdef FW_DEBUG_PIPELINE
                if (singleQueueOnly) vkQueuesByDeviceFamily_[deviceQueueDesc.first][j] = vkSingleQueue;
                else // i wonder if i can make this part even more unreadable ...
#endif
                    vkQueuesByDeviceFamily_[deviceQueueDesc.first][j] = vkDevice_->getQueue(deviceQueueDesc.first, j);
                vkQueuesByRequestedFamily_[mappings[j].first][mappings[j].second] = vkQueuesByDeviceFamily_[deviceQueueDesc.first][j];
                vkCmdPoolsByRequestedQFamily_[mappings[j].first] = *vkCmdPoolsByDeviceQFamily_[deviceQueueDesc.first];
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
        textureManager_ = std::make_unique<TextureManager>(this);
    }


    LogicalDevice::~LogicalDevice()
    {
        vkCmdPoolsByDeviceQFamily_.clear();
        vkCmdPoolsByRequestedQFamily_.clear();
        vkQueuesByDeviceFamily_.clear();
        vkQueuesByRequestedFamily_.clear();
    }

    PFN_vkVoidFunction LogicalDevice::LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory) const
    {
        auto func = vkGetDeviceProcAddr(static_cast<vk::Device>(*vkDevice_), functionName.c_str());
        if (func == nullptr) {
            if (mandatory) {
                LOG(FATAL) << "Could not load device function '" << functionName << "' [" << extensionName << "].";
                throw std::runtime_error("Could not load mandatory device function.");
            }
            LOG(WARNING) << "Could not load device function '" << functionName << "' [" << extensionName << "].";
        }

        return func;
    }

    std::unique_ptr<GraphicsPipeline> LogicalDevice::CreateGraphicsPipeline(const std::vector<std::string>& shaderNames,
        const glm::uvec2& size, unsigned int numBlendAttachments)
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

    constexpr std::size_t CalcAlignedSize(std::size_t size, std::size_t alignment)
    {
        return size + alignment - 1 - ((size + alignment - 1) % alignment);
    }

    std::size_t LogicalDevice::CalculateUniformBufferAlignment(std::size_t size) const
    {
        auto factor = vkPhysicalDeviceLimits_.minUniformBufferOffsetAlignment;
        return CalcAlignedSize(size, factor);
    }

    std::size_t LogicalDevice::CalculateBufferImageOffset(const Texture& second, std::size_t currentOffset) const
    {
        if (second.GetDescriptor().imageTiling_ == vk::ImageTiling::eOptimal)
            return CalcAlignedSize(currentOffset, vkPhysicalDeviceLimits_.bufferImageGranularity);
        return currentOffset;
    }
    std::size_t LogicalDevice::CalculateImageImageOffset(const Texture & first, const Texture & second, std::size_t currentOffset) const
    {
        if (first.GetDescriptor().imageTiling_ != second.GetDescriptor().imageTiling_)
            return CalcAlignedSize(currentOffset, vkPhysicalDeviceLimits_.bufferImageGranularity);
        return currentOffset;
    }
    std::pair<unsigned int, vk::Format> LogicalDevice::FindSupportedFormat(const std::vector<std::pair<unsigned int, vk::Format>>& candidates,
        vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
    {
        for (const auto& cand : candidates) {
            vk::FormatProperties formatProps = vkPhysicalDevice_.getFormatProperties(cand.second);

            if (tiling == vk::ImageTiling::eLinear && (formatProps.linearTilingFeatures & features) == features) return cand;
            else if (tiling == vk::ImageTiling::eOptimal && (formatProps.optimalTilingFeatures & features) == features) return cand;
        }

        throw std::runtime_error("No candidate format supported.");
    }
}
