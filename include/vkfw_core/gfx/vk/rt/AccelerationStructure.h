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
        AccelerationStructure(vkfw_core::gfx::LogicalDevice* device, vk::AccelerationStructureTypeKHR type,
                              vk::BuildAccelerationStructureFlagsKHR flags);
        AccelerationStructure(AccelerationStructure&& rhs);
        AccelerationStructure& operator=(AccelerationStructure&& rhs);
        ~AccelerationStructure();

        void AddGeometry(const vk::AccelerationStructureCreateGeometryTypeInfoKHR& typeInfo,
                         const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildOffsetInfoKHR& buildOffset);

        void BuildAccelerationStructure();

        const vk::AccelerationStructureKHR& GetAccelerationStructure() const { return m_vkAccelerationStructure.get(); }
        vk::DeviceAddress GetHandle() const { return m_handle; }

    private:
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> CreateAccelerationStructureScratchBuffer() const;
        void CreateAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& info);

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The acceleration structure type. */
        vk::AccelerationStructureTypeKHR m_type;
        /** The acceleration structure flags. */
        vk::BuildAccelerationStructureFlagsKHR m_flags;

        /** The actual vulkan acceleration structure object. */
        vk::UniqueAccelerationStructureKHR m_vkAccelerationStructure;
        /** The device memory containing the acceleration structure. */
        vk::UniqueDeviceMemory m_memory;
        /** The handle of the acceleration structure. */
        vk::DeviceAddress m_handle = 0;

        /** The geometry types contained in the acceleration structure. */
        std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR> m_geometryTypeInfos;
        /** The geometries contained in the acceleration structure. */
        std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
        /** The build offsets of the geometries. */
        std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> m_buildOffsets;
    };
}
