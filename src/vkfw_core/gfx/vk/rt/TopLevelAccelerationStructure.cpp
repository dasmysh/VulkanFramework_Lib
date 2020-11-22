/**
 * @file   TopLevelAccelerationStructure.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.22
 *
 * @brief  Implementation of the top level acceleration structure.
 */

#include "gfx/vk/rt/TopLevelAccelerationStructure.h"

namespace vkfw_core::gfx::rt {

    TopLevelAccelerationStructure::TopLevelAccelerationStructure(vkfw_core::gfx::LogicalDevice* device,
                                                                 vk::BuildAccelerationStructureFlagsKHR flags)
        : AccelerationStructure{device, vk::AccelerationStructureTypeKHR::eTopLevel, flags}
    {
    }

    TopLevelAccelerationStructure::TopLevelAccelerationStructure(TopLevelAccelerationStructure&& rhs) noexcept =
        default;

    TopLevelAccelerationStructure&
    TopLevelAccelerationStructure::operator=(TopLevelAccelerationStructure&& rhs) noexcept = default;

    TopLevelAccelerationStructure::~TopLevelAccelerationStructure() {}

    void TopLevelAccelerationStructure::AddBottomLevelAccelerationStructureInstance(
        const vk::AccelerationStructureInstanceKHR& blasInstance)
    {
        m_blasInstances.emplace_back(blasInstance);
    }

    void TopLevelAccelerationStructure::BuildAccelerationStructure()
    {
        vkfw_core::gfx::HostBuffer instancesBuffer{GetDevice(), vk::BufferUsageFlagBits::eShaderDeviceAddress};
        instancesBuffer.InitializeData(m_blasInstances);

        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo{
            vk::GeometryTypeKHR::eInstances, static_cast<std::uint32_t>(m_blasInstances.size()),
            vk::IndexType::eUint32,          0,
            vk::Format::eUndefined,          VK_FALSE};

        vk::AccelerationStructureGeometryInstancesDataKHR asGeometryDataInstances{
            VK_FALSE, instancesBuffer.GetDeviceAddressConst()};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataInstances};

        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eInstances, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{static_cast<std::uint32_t>(m_blasInstances.size()),
                                                                  0x0, 0, 0x0};

        AddGeometry(geometryTypeInfo, asGeometry, asBuildOffset);

        AccelerationStructure::BuildAccelerationStructure();
    }

}
