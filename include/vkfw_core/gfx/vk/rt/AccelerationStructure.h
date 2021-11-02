/**
 * @file   AccelerationStructure.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.21
 *
 * @brief  Declaration of the acceleration structure class.
 */

#pragma once

#include "main.h"
#include "gfx/vk/wrappers/VulkanObjectWrapper.h"
#include "gfx/vk/wrappers/PipelineBarriers.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
    class DeviceBuffer;
    struct BufferRange;
    class CommandBuffer;
}

namespace vkfw_core::gfx::rt {

    // TODO: add queue [9/20/2021 Sebastian Maisch]
    class AccelerationStructure : public VulkanObjectPrivateWrapper<vk::UniqueAccelerationStructureKHR>
    {
    public:
        AccelerationStructure(vkfw_core::gfx::LogicalDevice* device, std::string_view name,
                              vk::AccelerationStructureTypeKHR type,
                              vk::BuildAccelerationStructureFlagsKHR flags);
        AccelerationStructure(AccelerationStructure&& rhs) noexcept;
        AccelerationStructure& operator=(AccelerationStructure&& rhs) noexcept;
        virtual ~AccelerationStructure();

        virtual void BuildAccelerationStructure(CommandBuffer& cmdBuffer);

        void AccessBarrier(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                           PipelineBarrier& barrier) const;
        [[nodiscard]] vk::DeviceAddress GetAddressHandle(vk::AccessFlags2KHR access,
                                                         vk::PipelineStageFlags2KHR pipelineStages,
                                                         PipelineBarrier& barrier) const;
        [[nodiscard]] PipelineBarrier& GetBuildBarrier() { return m_buildBarrier; }

    protected:
        [[nodiscard]] vkfw_core::gfx::LogicalDevice* GetDevice() { return m_device; }
        void AddGeometry(const vk::AccelerationStructureGeometryKHR& geometry,
                         const vk::AccelerationStructureBuildRangeInfoKHR& buildRange);

    private:
        [[nodiscard]] std::unique_ptr<vkfw_core::gfx::DeviceBuffer> CreateAccelerationStructureScratchBuffer() const;
        void CreateAccelerationStructure(PipelineBarrier& barrier);

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The acceleration structure type. */
        vk::AccelerationStructureTypeKHR m_type;
        /** The acceleration structure flags. */
        vk::BuildAccelerationStructureFlagsKHR m_flags;
        /** The acceleration structures memory requirements. */
        vk::AccelerationStructureBuildSizesInfoKHR m_memoryRequirements;

        /** The device buffer containing the acceleration structure. */
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> m_buffer;
        /** The handle of the acceleration structure. */
        vk::DeviceAddress m_handle = 0;

        /** The geometries contained in the acceleration structure. */
        std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
        /** The build offsets of the geometries. */
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_buildRanges;
        /** The barrier to build the acceleration structure. */
        PipelineBarrier m_buildBarrier;
    };
}
