/**
 * @file   AccelerationStructure.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.21
 *
 * @brief  Declaration of the acceleration structure class.
 */

#pragma once

#include "main.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
    class DeviceBuffer;
}

namespace vkfw_core::gfx::rt {

    class AccelerationStructure
    {
    public:
        AccelerationStructure(vkfw_core::gfx::LogicalDevice* device);
        ~AccelerationStructure();

        void BuildAccelerationStructure(
            vk::AccelerationStructureBuildGeometryInfoKHR buildInfo,
            const vk::AccelerationStructureBuildOffsetInfoKHR* buildOffsets);

        const vk::AccelerationStructureKHR& GetAccelerationStructure() const { return m_vkAccelerationStructure.get(); }
        vk::DeviceAddress GetHandle() const { return m_handle; }

        void CreateAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& info);

    private:
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> CreateAccelerationStructureScratchBuffer() const;

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The actual vulkan acceleration structure object. */
        vk::UniqueAccelerationStructureKHR m_vkAccelerationStructure;
        /** The device memory containing the acceleration structure. */
        vk::UniqueDeviceMemory m_memory;
        /** The handle of the acceleration structure. */
        vk::DeviceAddress m_handle = 0;
    };
}
