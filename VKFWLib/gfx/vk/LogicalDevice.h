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

        class LogicalDevice
        {
        public:
            LogicalDevice(const vk::PhysicalDevice& phDevice, const vk::SurfaceKHR& surface = vk::SurfaceKHR());
            LogicalDevice(const LogicalDevice&);
            LogicalDevice(LogicalDevice&&);
            LogicalDevice& operator=(const LogicalDevice&) = delete;
            LogicalDevice& operator=(LogicalDevice&&) = delete;
            ~LogicalDevice();

        private:
            /** Holds the actual device. */
            vk::Device vkDevice_;
        };
    }
}
