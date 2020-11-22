/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructureGeometry::AccelerationStructureGeometry(vkfw_core::gfx::LogicalDevice* device)
        : m_device{device},
          m_BLAS{device, vk::AccelerationStructureTypeKHR::eBottomLevel,
                 vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace},
          m_TLAS{device, vk::AccelerationStructureTypeKHR::eTopLevel,
                 vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace}
    {
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::InitializeAccelerationStructure()
    {
        CreateBottomLevelAccelerationStructure();
        CreateTopLevelAccelerationStructure();
    }

    void AccelerationStructureGeometry::FillDescriptorLayoutBinding(vk::DescriptorSetLayoutBinding& asLayoutBinding,
                                                                    const vk::ShaderStageFlags& shaderFlags,
                                                                    std::uint32_t binding /*= 0*/) const
    {
        asLayoutBinding.setBinding(binding);
        asLayoutBinding.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        asLayoutBinding.setDescriptorCount(1);
        asLayoutBinding.setStageFlags(shaderFlags);
    }

    void AccelerationStructureGeometry::CreateLayout(vk::DescriptorPool descPool,
                                                     const vk::ShaderStageFlags& shaderFlags,
                                                     std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        vk::DescriptorSetLayoutBinding asLayoutBinding;
        FillDescriptorLayoutBinding(asLayoutBinding, shaderFlags, binding);

        vk::DescriptorSetLayoutCreateInfo asLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(), 1, &asLayoutBinding};
        m_internalDescLayout = m_device->GetDevice().createDescriptorSetLayoutUnique(asLayoutCreateInfo);
        m_descLayout = *m_internalDescLayout;
        AllocateDescriptorSet(descPool);
    }

    void AccelerationStructureGeometry::UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout,
                                                  std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        m_descLayout = usedLayout;
        AllocateDescriptorSet(descPool);
    }

    void AccelerationStructureGeometry::UseDescriptorSet(vk::DescriptorSet descSet, vk::DescriptorSetLayout usedLayout,
                                                         std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        m_descLayout = usedLayout;
        m_descSet = descSet;
    }

    void AccelerationStructureGeometry::FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const
    {
        descWrite.dstSet = m_descSet;
        descWrite.dstBinding = m_descBinding;
        descWrite.dstArrayElement = 0;
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
        descWrite.setPNext(&m_descriptorSetAccStructure);
    }

    void AccelerationStructureGeometry::AddTriangleGeometry(
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
            vk::Format::eR32G32B32Sfloat, vertexBufferDeviceAddress, vertexSize, vk::IndexType::eUint32,
            indexBufferDeviceAddress, transformDeviceAddress};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataTriangles};
        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eTriangles, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};
        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{static_cast<std::uint32_t>(primitiveCount), 0x0, 0,
                                                                  0x0};
        m_BLAS.AddGeometry(geometryTypeInfo, asGeometry, asBuildOffset);
    }

    void AccelerationStructureGeometry::CreateBottomLevelAccelerationStructure()
    {
        m_BLAS.BuildAccelerationStructure();
    }

    void AccelerationStructureGeometry::CreateTopLevelAccelerationStructure()
    {
        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo{
            vk::GeometryTypeKHR::eInstances, 1, vk::IndexType::eUint32, 0, vk::Format::eUndefined, VK_FALSE};

        vk::TransformMatrixKHR transform_matrix{std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f},
            std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f},
            std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}}};

        vk::AccelerationStructureInstanceKHR tlasInstance{
            transform_matrix, 0, 0xFF, 0, vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable, m_BLAS.GetHandle()};

        vkfw_core::gfx::HostBuffer instancesBuffer{m_device, vk::BufferUsageFlagBits::eShaderDeviceAddress};
        instancesBuffer.InitializeData(sizeof(vk::AccelerationStructureInstanceKHR),
                                       static_cast<const void*>(&tlasInstance));

        vk::AccelerationStructureGeometryInstancesDataKHR asGeometryDataInstances{
            VK_FALSE, instancesBuffer.GetDeviceAddressConst()};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataInstances};

        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eInstances, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{1, 0x0, 0, 0x0};

        m_TLAS.AddGeometry(geometryTypeInfo, asGeometry, asBuildOffset);
        m_TLAS.BuildAccelerationStructure();

        m_descriptorSetAccStructure.setAccelerationStructureCount(1);
        m_descriptorSetAccStructure.setPAccelerationStructures(&m_TLAS.GetAccelerationStructure());
    }

    void AccelerationStructureGeometry::AllocateDescriptorSet(vk::DescriptorPool descPool)
    {
        vk::DescriptorSetAllocateInfo descSetAllocInfo{descPool, 1, &m_descLayout};
        m_descSet = m_device->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
    }

}
