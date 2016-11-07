/**
 * @file   LogicalDevice.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Declaration of the LogicalDevice class representing a Vulkan device.
 */

#pragma once

#include "main.h"

namespace vku {

    class ShaderManager;

    namespace gfx {
        class GraphicsPipeline;
        class Framebuffer;

        struct DeviceQueueDesc
        {
            DeviceQueueDesc() = default;
            DeviceQueueDesc(uint32_t familyIndex, const std::vector<float>& priorities) : familyIndex_(familyIndex), priorities_(priorities) {}

            /** Holds the family index. */
            uint32_t familyIndex_;
            /** Holds the queues priorities. */
            std::vector<float> priorities_;
        };

        class LogicalDevice final
        {
        public:
            LogicalDevice(const vk::PhysicalDevice& phDevice, const std::vector<DeviceQueueDesc>& queueDescs, const vk::SurfaceKHR& surface = vk::SurfaceKHR());
            LogicalDevice(const LogicalDevice&); // TODO: implement [10/30/2016 Sebastian Maisch]
            LogicalDevice(LogicalDevice&&);
            LogicalDevice& operator=(const LogicalDevice&);
            LogicalDevice& operator=(LogicalDevice&&);
            ~LogicalDevice();


            const vk::PhysicalDevice& GetPhysicalDevice() const { return vkPhysicalDevice_; }
            const vk::Device& GetDevice() const { return vkDevice_; }
            const vk::Queue& GetQueue(unsigned int familyIndex, unsigned int queueIndex) const { return vkQueues_[familyIndex][queueIndex]; }
            const vk::CommandPool& GetCommandPool(unsigned int familyIndex) const { return vkCmdPools_[familyIndex]; }

            std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const std::vector<std::string>& shaderNames, const Framebuffer& fb, unsigned int numBlendAttachments);

            VkResult DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT* tagInfo) const;
            VkResult DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT* nameInfo) const;
            void CmdDebugMarkerBeginEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;
            void CmdDebugMarkerEndEXT(VkCommandBuffer cmdBuffer) const;
            void CmdDebugMarkerInsertEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;

            ShaderManager* GetShaderManager() const { return shaderManager_.get(); }

        private:
            PFN_vkVoidFunction LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory = false) const;

            /** Holds the physical device. */
            vk::PhysicalDevice vkPhysicalDevice_;
            /** Holds the actual device. */
            vk::Device vkDevice_;
            /** Holds the queues. */
            std::vector<std::vector<vk::Queue>> vkQueues_;
            /** Holds a command pool for each queue family. */
            std::vector<vk::CommandPool> vkCmdPools_;

            /** Holds whether debug markers are enabled. */
            bool enableDebugMarkers_ = false;

            // VK_EXT_debug_marker
            PFN_vkDebugMarkerSetObjectTagEXT fpDebugMarkerSetObjectTagEXT = nullptr;
            PFN_vkDebugMarkerSetObjectNameEXT fpDebugMarkerSetObjectNameEXT = nullptr;
            PFN_vkCmdDebugMarkerBeginEXT fpCmdDebugMarkerBeginEXT = nullptr;
            PFN_vkCmdDebugMarkerEndEXT fpCmdDebugMarkerEndEXT = nullptr;
            PFN_vkCmdDebugMarkerInsertEXT fpCmdDebugMarkerInsertEXT = nullptr;

            /** Holds the shader manager. */
            std::unique_ptr<ShaderManager> shaderManager_;
        };
    }
}
