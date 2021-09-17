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
        AccelerationStructure(AccelerationStructure&& rhs) noexcept;
        AccelerationStructure& operator=(AccelerationStructure&& rhs) noexcept;
        virtual ~AccelerationStructure();

        virtual void BuildAccelerationStructure();

        const vk::AccelerationStructureKHR& GetAccelerationStructure() const { return m_vkAccelerationStructure.get(); }
        vk::DeviceAddress GetHandle() const { return m_handle; }

    protected:
        vkfw_core::gfx::LogicalDevice* GetDevice() { return m_device; }
        void AddGeometry(const vk::AccelerationStructureGeometryKHR& geometry,
                         const vk::AccelerationStructureBuildRangeInfoKHR& buildRange);

    private:
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> CreateAccelerationStructureScratchBuffer() const;
        void CreateAccelerationStructure();

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The acceleration structure type. */
        vk::AccelerationStructureTypeKHR m_type;
        /** The acceleration structure flags. */
        vk::BuildAccelerationStructureFlagsKHR m_flags;
        /** The acceleration structures memory requirements. */
        vk::AccelerationStructureBuildSizesInfoKHR m_memoryRequirements;

        /** The actual vulkan acceleration structure object. */
        vk::UniqueAccelerationStructureKHR m_vkAccelerationStructure;
        /** The device buffer containing the acceleration structure. */
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> m_buffer;
        /** The handle of the acceleration structure. */
        vk::DeviceAddress m_handle = 0;

        /** The geometries contained in the acceleration structure. */
        std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
        /** The build offsets of the geometries. */
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_buildRanges;
    };
}
