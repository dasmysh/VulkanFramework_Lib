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
                                                                 std::string_view name,
                                                                 vk::BuildAccelerationStructureFlagsKHR flags)
        : AccelerationStructure{device, name, vk::AccelerationStructureTypeKHR::eTopLevel, flags}
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
        vkfw_core::gfx::HostBuffer instancesBuffer{
            GetDevice(), fmt::format("InstanceBuffer:{}", GetName()),
            vk::BufferUsageFlagBits::eShaderDeviceAddress
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR};
        instancesBuffer.InitializeData(m_blasInstances);

        vk::AccelerationStructureGeometryInstancesDataKHR asGeometryDataInstances{
            VK_FALSE, instancesBuffer.GetDeviceAddressConst()};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataInstances};

        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eInstances, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildRangeInfoKHR asBuildRange{static_cast<std::uint32_t>(m_blasInstances.size()),
                                                                  0x0, 0, 0x0};

        AddGeometry(asGeometry, asBuildRange);

        AccelerationStructure::BuildAccelerationStructure();
    }

}
