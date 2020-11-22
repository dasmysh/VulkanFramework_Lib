/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include <gfx/meshes/MeshInfo.h>
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructureGeometry::AccelerationStructureGeometry(vkfw_core::gfx::LogicalDevice* device)
        : m_device{device},
          m_TLAS{device, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace},
          m_memGroup{m_device, vk::MemoryPropertyFlags()}
    {
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh,
                                                        const glm::mat4& transform, std::size_t vertexSize,
                                                        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                                        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress)
    {
        AddMeshNodeGeometry(mesh, mesh.GetRootNode(), transform, vertexSize, vertexBufferDeviceAddress, indexBufferDeviceAddress);
    }

    void AccelerationStructureGeometry::AddMeshNodeGeometry(const vkfw_core::gfx::MeshInfo& mesh,
                                                            const vkfw_core::gfx::SceneMeshNode* node,
                                                            const glm::mat4& transform, std::size_t vertexSize,
                                                            vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                                            vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress)
    {
        if (!node->HasMeshes()) { return; }

        auto localTransform = transform * node->GetLocalTransform();
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            AddSubMeshGeometry(mesh.GetSubMeshes()[node->GetSubMeshID(i)], localTransform, vertexSize,
                               vertexBufferDeviceAddress, indexBufferDeviceAddress);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            AddMeshNodeGeometry(mesh, node->GetChild(i), localTransform, vertexSize, vertexBufferDeviceAddress,
                                indexBufferDeviceAddress);
        }
    }

    void AccelerationStructureGeometry::AddSubMeshGeometry(const vkfw_core::gfx::SubMesh& subMesh,
                                                           const glm::mat4& transform, std::size_t vertexSize,
                                                           vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                                           vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress)
    {
        vk::DeviceOrHostAddressConstKHR indexBufferAddress{indexBufferDeviceAddress.deviceAddress
                                                           + subMesh.GetIndexOffset() * sizeof(std::uint32_t)};

        auto blasIndex = AddBottomLevelAccelerationStructure(glm::transpose(transform));
        m_BLAS[blasIndex].AddTriangleGeometry(subMesh.GetNumberOfTriangles(), subMesh.GetNumberOfIndices(), vertexSize,
                                              vertexBufferDeviceAddress, indexBufferAddress);
    }

    void AccelerationStructureGeometry::BuildAccelerationStructure()
    {
        // TODO: a way to build all BLAS at once would be good. [11/22/2020 Sebastian Maisch]
        for (std::size_t i = 0; i < m_BLAS.size(); ++i) {
            m_BLAS[i].BuildAccelerationStructure();

            vk::AccelerationStructureInstanceKHR blasInstance{
                vk::TransformMatrixKHR{}, 0, 0xFF, 0, vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
                m_BLAS[i].GetHandle()};
            memcpy(&blasInstance.transform, &m_BLASTransforms[i], sizeof(glm::mat3x4));
            m_TLAS.AddBottomLevelAccelerationStructureInstance(blasInstance);
        }

        m_TLAS.BuildAccelerationStructure();

        m_descriptorSetAccStructure.setAccelerationStructureCount(1);
        m_descriptorSetAccStructure.setPAccelerationStructures(&m_TLAS.GetAccelerationStructure());
    }

    std::size_t AccelerationStructureGeometry::AddBottomLevelAccelerationStructure(const glm::mat3x4& transform)
    {
        auto result = m_BLAS.size();
        m_BLAS.emplace_back(m_device, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        return result;
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
