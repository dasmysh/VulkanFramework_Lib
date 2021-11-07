/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/meshes/MeshInfo.h"
#include "gfx/vk/QueuedDeviceTransfer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructureGeometry::AccelerationStructureGeometry(LogicalDevice* device, std::string_view name,
                                                                 const std::vector<std::uint32_t>& queueFamilyIndices)
        : m_device{device}
        , m_name{name}
        , m_queueFamilyIndices{queueFamilyIndices}
        , m_TLAS{device, fmt::format("TLAS:", name), vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace}
        , m_bufferMemGroup{m_device, fmt::format("ASBufferMemGroup:{}", name), vk::MemoryPropertyFlags{}}
        , m_textureMemGroup{m_device, fmt::format("ASTextureMemGroup:{}", name), vk::MemoryPropertyFlags{}}
    {
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform)
    {
        m_meshGeometryInfos.emplace_back(m_geometryIndex++, &mesh, transform);
    }

    void AccelerationStructureGeometry::AddInstanceBufferAndTransferMemGroup()
    {
        m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, m_instanceBufferOffset, m_instanceInfos);
        m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, m_materialBufferOffset, m_materialInfos);
        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, m_device->GetQueue(0, 0)};
        m_bufferMemGroup.FinalizeDeviceGroup();
        m_bufferMemGroup.TransferData(transfer);
        m_textureMemGroup.FinalizeDeviceGroup();
        m_textureMemGroup.TransferData(transfer);
        transfer.FinishTransfer();
    }

    void AccelerationStructureGeometry::AddMeshNodeInstance(const MeshGeometryInfo& mesh, const SceneMeshNode* node,
                                                            const glm::mat4& transform, std::uint32_t materialOffset)
    {
        if (!node->HasMeshes()) { return; }

        auto localTransform = transform * node->GetLocalTransform();
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            auto meshMaterialId = mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)].GetMaterialID();
            auto materialIndex = meshMaterialId + materialOffset;
            AddInstanceInfo(static_cast<std::uint32_t>(mesh.vertexSize), static_cast<std::uint32_t>(mesh.index),
                            materialIndex, transform,
                            mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)].GetIndexOffset());
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            AddMeshNodeInstance(mesh, node->GetChild(i), localTransform, materialOffset);
        }
    }

    void AccelerationStructureGeometry::AddMeshNodeGeometry(const MeshGeometryInfo& mesh, const SceneMeshNode* node,
                                                            const glm::mat4& transform)
    {
        if (!node->HasMeshes()) { return; }

        auto localTransform = transform * node->GetLocalTransform();
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            AddSubMeshGeometry(mesh, mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)], localTransform);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            AddMeshNodeGeometry(mesh, node->GetChild(i), localTransform);
        }
    }

    void AccelerationStructureGeometry::AddSubMeshGeometry(const MeshGeometryInfo& mesh, const SubMesh& subMesh,
                                                           const glm::mat4& transform)
    {
        auto blasIndex =
            AddBottomLevelAccelerationStructure(static_cast<std::uint32_t>(mesh.index), glm::transpose(transform));

        vk::DeviceOrHostAddressConstKHR bufferDeviceAddress =
            m_bufferMemGroup.GetBuffer(m_bufferIndex)
                ->GetDeviceAddressConst(vk::AccessFlagBits2KHR::eAccelerationStructureRead,
                                        vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuild,
                                        m_BLAS[blasIndex].GetBuildBarrier());
        vk::DeviceOrHostAddressConstKHR vboDeviceAddress = bufferDeviceAddress.deviceAddress + mesh.vboOffset;
        vk::DeviceOrHostAddressConstKHR iboDeviceAddress = bufferDeviceAddress.deviceAddress + mesh.iboOffset;

        vk::DeviceOrHostAddressConstKHR indexBufferAddress{iboDeviceAddress.deviceAddress
                                                           + subMesh.GetIndexOffset() * sizeof(std::uint32_t)};

        m_BLAS[blasIndex].AddTriangleGeometry(
            subMesh.GetNumberOfTriangles(), subMesh.GetNumberOfIndices(), mesh.vertexSize,
            mesh.mesh->GetMaterial(subMesh.GetMaterialID())->m_hasAlpha, vboDeviceAddress, indexBufferAddress);
    }

    void AccelerationStructureGeometry::AddInstanceInfo(std::uint32_t vertexSize, std::uint32_t bufferIndex,
                                                        std::uint32_t materialIndex, const glm::mat4& transform,
                                                        std::uint32_t indexOffset /*= 0*/)
    {
        auto& instanceInfo =
            m_instanceInfos.emplace_back(vertexSize, bufferIndex, materialIndex, indexOffset);
        instanceInfo.transform = transform;
        instanceInfo.transformInverseTranspose =
            glm::mat4(glm::mat3(glm::transpose(glm::inverse(instanceInfo.transform))));
    }

    void AccelerationStructureGeometry::AddTriangleGeometry(
        const glm::mat4& transform, const MaterialInfo& materialInfo, std::size_t primitiveCount,
        std::size_t vertexCount, std::size_t vertexSize, DeviceBuffer* vbo, std::size_t vboOffset /* = 0*/,
        DeviceBuffer* ibo /*= nullptr*/, std::size_t iboOffset /*= 0*/)
    {
        auto blasIndex =
            AddBottomLevelAccelerationStructure(static_cast<std::uint32_t>(m_geometryIndex), glm::transpose(transform));

        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress =
            vbo->GetDeviceAddressConst(vk::AccessFlagBits2KHR::eAccelerationStructureRead,
                                       vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuild,
                                       m_BLAS[blasIndex].GetBuildBarrier())
                .deviceAddress
            + vboOffset;
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress = vertexBufferDeviceAddress;
        if (ibo != nullptr) {
            indexBufferDeviceAddress =
                ibo->GetDeviceAddressConst(vk::AccessFlagBits2KHR::eAccelerationStructureRead,
                                           vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuild,
                                           m_BLAS[blasIndex].GetBuildBarrier())
                    .deviceAddress
                + iboOffset;
        } else {
            if (iboOffset == 0) {
                iboOffset = m_device->CalculateStorageBufferAlignment(vboOffset + vertexCount * vertexSize);
            }
            indexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress + iboOffset;
        }


        std::uint32_t materialIndex = static_cast<std::uint32_t>(m_materials.size());
        m_materials.emplace_back(&materialInfo, m_device, m_textureMemGroup, m_queueFamilyIndices);
        AddInstanceInfo(static_cast<std::uint32_t>(vertexSize), static_cast<std::uint32_t>(m_geometryIndex),
                        materialIndex, transform);
        m_BLAS[blasIndex].AddTriangleGeometry(primitiveCount, vertexCount, vertexSize, false, vertexBufferDeviceAddress,
                                              indexBufferDeviceAddress);
        m_triangleGeometryInfos.emplace_back(m_geometryIndex++, vbo, vboOffset, vertexCount * vertexSize,
                                             ibo ? ibo : vbo, iboOffset, primitiveCount * 3 * sizeof(std::uint32_t));
    }

    void AccelerationStructureGeometry::BuildAccelerationStructure()
    {
        PipelineBarrier barrier{m_device};
        // TODO: make the queue family a parameter.
        auto cmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(m_device, "ASBuildCmdBuffer", "ASBuild",
                                                                              m_device->GetCommandPool(0));

        for (std::size_t i = 0; i < m_BLAS.size(); ++i) {
            m_BLAS[i].BuildAccelerationStructure(cmdBuffer);

            vk::AccelerationStructureInstanceKHR blasInstance{
                vk::TransformMatrixKHR{},
                m_bufferIndices[i],
                0xFF,
                0,
                vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
                m_BLAS[i].GetAddressHandle(vk::AccessFlagBits2KHR::eAccelerationStructureRead,
                                           vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuild, barrier)};
            memcpy(&blasInstance.transform, &m_BLASTransforms[i], sizeof(glm::mat3x4));
            m_TLAS.AddBottomLevelAccelerationStructureInstance(blasInstance);
        }
        barrier.Record(cmdBuffer);

        m_TLAS.BuildAccelerationStructure(cmdBuffer);
        auto fence = vkfw_core::gfx::CommandBuffer::endSingleTimeSubmit(m_device->GetQueue(0, 0), cmdBuffer, {}, {});
        fence->Wait(m_device, defaultFenceTimeout);

        for (auto& blas : m_BLAS) { blas.FinalizeBuild(); }
        m_TLAS.FinalizeBuild();
    }

    std::size_t AccelerationStructureGeometry::AddBottomLevelAccelerationStructure(std::uint32_t bufferIndex,
                                                                                   const glm::mat3x4& transform)
    {
        auto blasIndex = m_BLAS.size();
        m_BLAS.emplace_back(m_device, fmt::format("BLAS:{}-{}", m_name, blasIndex),
                            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        m_bufferIndices.push_back(bufferIndex);
        return blasIndex;
    }

    void AccelerationStructureGeometry::AddDescriptorLayoutBindingAS(DescriptorSetLayout& layout,
                                                                     vk::ShaderStageFlags shaderFlags,
                                                                     std::uint32_t bindingAS)
    {
        layout.AddBinding(bindingAS, vk::DescriptorType::eAccelerationStructureKHR, 1, shaderFlags);
    }

    void AccelerationStructureGeometry::AddDescriptorLayoutBindingBuffers(
        DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags, std::uint32_t bindingVBO,
        std::uint32_t bindingIBO, std::uint32_t bindingInstanceBuffer, std::uint32_t bindingMaterialBuffer,
        std::uint32_t bindingDiffuseTexture,
        std::uint32_t bindingBumpTexture)
    {
        layout.AddBinding(bindingVBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingIBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingInstanceBuffer, vk::DescriptorType::eStorageBuffer, 1, shaderFlags);
        layout.AddBinding(bindingMaterialBuffer, vk::DescriptorType::eStorageBuffer, 1, shaderFlags);
        layout.AddBinding(bindingDiffuseTexture, vk::DescriptorType::eCombinedImageSampler,
                          static_cast<std::uint32_t>(m_diffuseTextures.size()), shaderFlags);
        layout.AddBinding(bindingBumpTexture, vk::DescriptorType::eCombinedImageSampler,
                          static_cast<std::uint32_t>(m_bumpMaps.size()), shaderFlags);
    }

    vk::AccelerationStructureKHR AccelerationStructureGeometry::GetTopLevelAccelerationStructure(
        vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages, PipelineBarrier& barrier) const
    {
        for (const auto& blas : m_BLAS) { blas.AccessBarrier(access, pipelineStages, barrier); }
        return m_TLAS.GetAccelerationStructure(access, pipelineStages, barrier);
    }

    void AccelerationStructureGeometry::FillGeometryInfo(std::vector<BufferRange>& vbos, std::vector<BufferRange>& ibos,
                                                         BufferRange& instanceBuffer)
    {
        auto vboIOffset = vbos.size();
        auto iboIOffset = ibos.size();
        vbos.resize(vboIOffset + m_triangleGeometryInfos.size() + m_meshGeometryInfos.size());
        ibos.resize(iboIOffset + m_triangleGeometryInfos.size() + m_meshGeometryInfos.size());

        for (const auto& triangleGeometry : m_triangleGeometryInfos) {
            vbos[vboIOffset + triangleGeometry.index] =
                BufferRange{triangleGeometry.vboBuffer, triangleGeometry.vboOffset, triangleGeometry.vboRange};
            ibos[iboIOffset + triangleGeometry.index] =
                BufferRange{triangleGeometry.iboBuffer, triangleGeometry.iboOffset, triangleGeometry.iboRange};
        }

        for (const auto& meshGeometry : m_meshGeometryInfos) {
            vbos[vboIOffset + meshGeometry.index] =
                BufferRange{m_bufferMemGroup.GetBuffer(m_bufferIndex), meshGeometry.vboOffset, meshGeometry.vboRange};
            ibos[iboIOffset + meshGeometry.index] =
                BufferRange{m_bufferMemGroup.GetBuffer(m_bufferIndex), meshGeometry.iboOffset, meshGeometry.iboRange};
        }

        instanceBuffer.m_buffer = m_bufferMemGroup.GetBuffer(m_bufferIndex);
        instanceBuffer.m_offset = m_instanceBufferOffset;
        instanceBuffer.m_range = m_instanceBufferRange;
    }

    void AccelerationStructureGeometry::FillMaterialInfo(BufferRange& materialBuffer,
                                                         std::vector<Texture*>& diffuseTextures,
                                                         std::vector<Texture*>& bumpMaps)
    {
        materialBuffer.m_buffer = m_bufferMemGroup.GetBuffer(m_bufferIndex);
        materialBuffer.m_offset = m_materialBufferOffset;
        materialBuffer.m_range = m_materialBufferRange;

        for (auto tex : m_diffuseTextures) { diffuseTextures.emplace_back(tex); }

        for (auto tex : m_bumpMaps) { bumpMaps.emplace_back(tex); }
    }

    void AccelerationStructureGeometry::CreateResourceUseBarriers(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStage, vk::ImageLayout newLayout, PipelineBarrier& barrier)
    {
        // TODO: buffer barriers.

        for (auto& mat : m_materials) {
            if (mat.m_diffuseTexture) {
                mat.m_diffuseTexture->GetTexture().AccessBarrier(access, pipelineStage, newLayout, barrier);
            }

            if (mat.m_bumpMap) {
                mat.m_bumpMap->GetTexture().AccessBarrier(access, pipelineStage, newLayout, barrier);
            }
        }
    }

}
