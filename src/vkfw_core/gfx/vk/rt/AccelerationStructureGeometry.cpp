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
          m_TLAS{device, vk::AccelerationStructureTypeKHR::eTopLevel,
                 vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace}
    {
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::BuildAccelerationStructure()
    {
        vkfw_core::gfx::HostBuffer instancesBuffer{m_device, vk::BufferUsageFlagBits::eShaderDeviceAddress};
        std::vector<vk::AccelerationStructureInstanceKHR> tlasInstances;

        // TODO: a way to build all BLAS at once would be good. [11/22/2020 Sebastian Maisch]
        for (std::size_t i = 0; i < m_BLAS.size(); ++i) {
            m_BLAS[i].BuildAccelerationStructure();

            auto& tlasInstance = tlasInstances.emplace_back(vk::TransformMatrixKHR{}, 0, 0xFF, 0,
                                                            vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
                                                            m_BLAS[i].GetHandle());

            memcpy(&tlasInstance.transform, &m_BLASTransforms[i], sizeof(glm::mat3x4));
        }

        instancesBuffer.InitializeData(tlasInstances);

        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo{
            vk::GeometryTypeKHR::eInstances, static_cast<std::uint32_t>(m_BLAS.size()),
            vk::IndexType::eUint32,          0,
            vk::Format::eUndefined,          VK_FALSE};

        vk::AccelerationStructureGeometryInstancesDataKHR asGeometryDataInstances{
            VK_FALSE, instancesBuffer.GetDeviceAddressConst()};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataInstances};

        vk::AccelerationStructureGeometryKHR asGeometry{vk::GeometryTypeKHR::eInstances, asGeometryData,
                                                        vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{static_cast<std::uint32_t>(m_BLAS.size()), 0x0, 0,
                                                                  0x0};

        m_TLAS.AddGeometry(geometryTypeInfo, asGeometry, asBuildOffset);
        CreateTopLevelAccelerationStructure();
    }

    std::size_t AccelerationStructureGeometry::AddBottomLevelAccelerationStructure(const glm::mat3x4& transform)
    {
        auto result = m_BLAS.size();
        m_BLAS.emplace_back(m_device, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        return result;
    }

    void AccelerationStructureGeometry::CreateTopLevelAccelerationStructure()
    {
        m_TLAS.BuildAccelerationStructure();

        m_descriptorSetAccStructure.setAccelerationStructureCount(1);
        m_descriptorSetAccStructure.setPAccelerationStructures(&m_TLAS.GetAccelerationStructure());
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

    void AccelerationStructureGeometry::AllocateDescriptorSet(vk::DescriptorPool descPool)
    {
        vk::DescriptorSetAllocateInfo descSetAllocInfo{descPool, 1, &m_descLayout};
        m_descSet = m_device->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
    }

}
