/**
 * @file   BottomLevelAccelerationStructure.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.22
 *
 * @brief  Implementation of the bottom level acceleration structure.
 */

#include "gfx/vk/rt/BottomLevelAccelerationStructure.h"

namespace vkfw_core::gfx::rt {

    BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(LogicalDevice* device,
                                                                       vk::BuildAccelerationStructureFlagsKHR flags)
        : AccelerationStructure{device, vk::AccelerationStructureTypeKHR::eBottomLevel, flags}
    {
    }

    BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(BottomLevelAccelerationStructure&& rhs) =
        default;

    BottomLevelAccelerationStructure&
    BottomLevelAccelerationStructure::operator=(BottomLevelAccelerationStructure&& rhs) = default;

    BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure() {}

    void BottomLevelAccelerationStructure::AddTriangleGeometry(
        std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress,
        vk::DeviceOrHostAddressConstKHR transformDeviceAddress /*= nullptr*/)
    {
        vk::Bool32 hasTransform = (transformDeviceAddress.hostAddress == nullptr) ? VK_FALSE : VK_TRUE;
        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo{
            vk::GeometryTypeKHR::eTriangles, static_cast<std::uint32_t>(primitiveCount),
            vk::IndexType::eUint32,          static_cast<std::uint32_t>(vertexCount),
            vk::Format::eR32G32B32Sfloat,    hasTransform};

        vk::AccelerationStructureGeometryTrianglesDataKHR asGeometryDataTriangles{
            vk::Format::eR32G32B32Sfloat, vertexBufferDeviceAddress, vertexSize,
            vk::IndexType::eUint32,       indexBufferDeviceAddress,  transformDeviceAddress};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataTriangles};
        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eTriangles, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};
        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{static_cast<std::uint32_t>(primitiveCount), 0x0, 0,
                                                                  0x0};
        AddGeometry(geometryTypeInfo, asGeometry, asBuildOffset);
    }

}
