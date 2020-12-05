/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include <gfx/meshes/MeshInfo.h>
#include "gfx/vk/QueuedDeviceTransfer.h"
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

    void AccelerationStructureGeometry::AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh,
                                                        const glm::mat4& transform)
    {
        m_meshGeometryInfos.emplace_back(&mesh, transform);
    }

    void AccelerationStructureGeometry::FinalizeMeshGeometry()
    {
        for (auto& meshInfo : m_meshGeometryInfos) {
            const std::size_t bufferSize = meshInfo.iboOffset + meshInfo.iboRange;
            meshInfo.bufferIndex = m_memGroup.AddBufferToGroup(
                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                    | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                bufferSize, std::vector<std::uint32_t>{{0, 1}});
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.vboOffset, meshInfo.vertices);
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.iboOffset, meshInfo.indices);
        }

        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, std::make_pair(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        for (auto& meshInfo : m_meshGeometryInfos) {
            // free some memory.
            meshInfo.indices.clear();
            meshInfo.vertices.clear();

            vk::DeviceOrHostAddressConstKHR bufferDeviceAddress =
                m_memGroup.GetBuffer(meshInfo.bufferIndex)->GetDeviceAddressConst();
            vk::DeviceOrHostAddressConstKHR vboDeviceAddress = bufferDeviceAddress.deviceAddress + meshInfo.vboOffset;
            vk::DeviceOrHostAddressConstKHR iboDeviceAddress = bufferDeviceAddress.deviceAddress + meshInfo.iboOffset;

            AddMeshGeometry(*meshInfo.mesh, meshInfo.transform, meshInfo.vertexSize, vboDeviceAddress, iboDeviceAddress);
        }
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

    void AccelerationStructureGeometry::AddTriangleGeometryToBLAS(
        std::size_t blasIndex, std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
        const DeviceBuffer* vbo, std::size_t vboOffset, const DeviceBuffer* ibo /*= nullptr*/,
        std::size_t iboOffset /*= 0*/, vk::DeviceOrHostAddressConstKHR transformDeviceAddress /*= nullptr*/)
    {
        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress =
            vbo->GetDeviceAddressConst().deviceAddress + vboOffset;
        auto vboBuffer = vbo->GetBuffer();
        auto iboBuffer = vboBuffer;
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress =
            vbo->GetDeviceAddressConst().deviceAddress + iboOffset;
        if (ibo != nullptr) {
            iboBuffer = ibo->GetBuffer();
            indexBufferDeviceAddress = ibo->GetDeviceAddressConst().deviceAddress + iboOffset;
        }

        m_BLAS[blasIndex].AddTriangleGeometry(primitiveCount, vertexCount, vertexSize, vertexBufferDeviceAddress,
                                              indexBufferDeviceAddress, transformDeviceAddress);
        m_triangleGeometryInfos.emplace_back(vboBuffer, vboOffset, vertexCount * vertexSize, iboBuffer, iboOffset,
                                             primitiveCount * 3 * sizeof(std::uint32_t));
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
    }

    std::size_t AccelerationStructureGeometry::AddBottomLevelAccelerationStructure(const glm::mat3x4& transform)
    {
        auto result = m_BLAS.size();
        m_BLAS.emplace_back(m_device, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        return result;
    }

    std::uint32_t AccelerationStructureGeometry::AddDescriptorLayoutBinding(DescriptorSetLayout& layout,
                                                                            vk::ShaderStageFlags shaderFlags,
                                                                            std::uint32_t bindingStart /*= 0*/)
    {
        layout.AddBinding(bindingStart++, vk::DescriptorType::eAccelerationStructureKHR, 1, shaderFlags);
        layout.AddBinding(bindingStart++, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingStart++, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        return bindingStart;
    }

    void AccelerationStructureGeometry::FillDescriptorAccelerationStructureInfo(
        vk::WriteDescriptorSetAccelerationStructureKHR& descInfo) const
    {
        descInfo.setAccelerationStructureCount(1);
        descInfo.setPAccelerationStructures(&m_TLAS.GetAccelerationStructure());
    }

    void AccelerationStructureGeometry::FillDescriptorBufferInfo(vk::DescriptorBufferInfo& /*vboBufferInfo*/,
                                                                      vk::DescriptorBufferInfo& /*iboBufferInfo*/) const
    {
        // vboBufferInfo.setBuffer(m_memGroup.GetBuffer(m_geometryBufferIndex_deprecated)->GetBuffer());
        // vboBufferInfo.offset = ;
        // vboBufferInfo.range = ;

        // iboBufferInfo.setBuffer(m_memGroup.GetBuffer(m_geometryBufferIndex_deprecated)->GetBuffer());
        // iboBufferInfo.offset = ;
        // iboBufferInfo.range = ;
    }

}
