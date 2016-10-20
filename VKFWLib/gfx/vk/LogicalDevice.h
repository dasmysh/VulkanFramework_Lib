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
    namespace gfx {

        struct DeviceQueueDesc
        {
            /** Holds the family index. */
            uint32_t familyIndex_;
            /** Holds the queues priorities. */
            std::vector<float> priorities_;
        };

        class LogicalDevice
        {
        public:
            LogicalDevice(const vk::PhysicalDevice& phDevice, const std::vector<DeviceQueueDesc>& queueDescs, const vk::SurfaceKHR& surface = vk::SurfaceKHR());
            LogicalDevice(const LogicalDevice&);
            LogicalDevice(LogicalDevice&&);
            LogicalDevice& operator=(const LogicalDevice&);
            LogicalDevice& operator=(LogicalDevice&&);
            ~LogicalDevice();

            VkResult DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT* tagInfo) const;
            VkResult DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT* nameInfo) const;
            void CmdDebugMarkerBeginEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;
            void CmdDebugMarkerEndEXT(VkCommandBuffer cmdBuffer) const;
            void CmdDebugMarkerInsertEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;

        private:
            PFN_vkVoidFunction LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory = false) const;

            /** Holds the actual device. */
            vk::Device vkDevice_;
            /** Holds the queues. */
            std::vector<std::vector<vk::Queue>> vkQueues_;

            /** Holds whether debug markers are enabled. */
            bool enableDebugMarkers_ = false;

            // VK_EXT_debug_marker
            PFN_vkDebugMarkerSetObjectTagEXT fpDebugMarkerSetObjectTagEXT = nullptr;
            PFN_vkDebugMarkerSetObjectNameEXT fpDebugMarkerSetObjectNameEXT = nullptr;
            PFN_vkCmdDebugMarkerBeginEXT fpCmdDebugMarkerBeginEXT = nullptr;
            PFN_vkCmdDebugMarkerEndEXT fpCmdDebugMarkerEndEXT = nullptr;
            PFN_vkCmdDebugMarkerInsertEXT fpCmdDebugMarkerInsertEXT = nullptr;
        };
    }
}
