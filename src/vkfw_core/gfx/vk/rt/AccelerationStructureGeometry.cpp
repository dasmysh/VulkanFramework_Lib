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

    void AccelerationStructureGeometry::TransferMemGroup()
    {
        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, m_device->GetQueue(0, 0)};
        m_bufferMemGroup.FinalizeDeviceGroup();
        m_bufferMemGroup.TransferData(transfer);
        m_textureMemGroup.FinalizeDeviceGroup();
        m_textureMemGroup.TransferData(transfer);
        transfer.FinishTransfer();
    }

    void AccelerationStructureGeometry::AddMeshNodeInstance(
        const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform,
        const std::vector<std::pair<std::uint32_t, std::uint32_t>>& materialMapping)
    {
        if (!node->HasMeshes()) { return; }

        auto localTransform = transform * node->GetLocalTransform();
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            auto meshMaterialId = mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)].GetMaterialID();
            auto [materialType, materialIndex] = materialMapping[meshMaterialId];
            AddInstanceInfo(static_cast<std::uint32_t>(mesh.vertexSize), static_cast<std::uint32_t>(mesh.index),
                            materialType, materialIndex, transform,
                            mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)].GetIndexOffset());
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            AddMeshNodeInstance(mesh, node->GetChild(i), localTransform, materialMapping);
        }
    }

    void AccelerationStructureGeometry::AddMeshNodeGeometry(const MeshGeometryInfo& mesh, const SceneMeshNode* node,
                                                            const glm::mat4& transform,
                                                            const std::vector<std::uint32_t>& materialSBTMapping)
    {
        if (!node->HasMeshes()) { return; }

        auto localTransform = transform * node->GetLocalTransform();
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            AddSubMeshGeometry(mesh, mesh.mesh->GetSubMeshes()[node->GetSubMeshID(i)], localTransform,
                               materialSBTMapping);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            AddMeshNodeGeometry(mesh, node->GetChild(i), localTransform, materialSBTMapping);
        }
    }

    void AccelerationStructureGeometry::AddSubMeshGeometry(const MeshGeometryInfo& mesh, const SubMesh& subMesh,
                                                           const glm::mat4& transform,
                                                           const std::vector<std::uint32_t>& materialSBTMapping)
    {
        auto sbtInstanceOffset =
            materialSBTMapping[mesh.mesh->GetMaterial(subMesh.GetMaterialID())->m_materialIdentifier];
        auto blasIndex = AddBottomLevelAccelerationStructure(static_cast<std::uint32_t>(mesh.index), sbtInstanceOffset,
                                                             glm::transpose(transform));

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

    std::pair<std::uint32_t, std::uint32_t> AccelerationStructureGeometry::AddMaterial(const MaterialInfo& materialInfo)
    {
        std::uint32_t materialType = materialInfo.m_materialIdentifier;
        if (m_materials.size() <= materialType) { m_materials.resize(materialType + 1); }

        std::uint32_t materialIndex = static_cast<std::uint32_t>(m_materials[materialType].size());
        m_materials[materialType].emplace_back(&materialInfo, m_device, m_textureMemGroup, m_queueFamilyIndices);
        return { materialType, materialIndex };
    }

    void AccelerationStructureGeometry::AddInstanceInfo(std::uint32_t vertexSize, std::uint32_t bufferIndex,
                                                        std::uint32_t materialType, std::uint32_t materialIndex,
                                                        const glm::mat4& transform, std::uint32_t indexOffset /*= 0*/)
    {
        auto& instanceInfo =
            m_instanceInfos.emplace_back(vertexSize, bufferIndex, materialType, materialIndex, indexOffset);
        instanceInfo.transform = transform;
        instanceInfo.transformInverseTranspose =
            glm::mat4(glm::mat3(glm::transpose(glm::inverse(instanceInfo.transform))));
    }

    void AccelerationStructureGeometry::AddTriangleGeometry(
        const glm::mat4& transform, const MaterialInfo& materialInfo,
        const std::vector<std::uint32_t>& materialSBTMapping, std::size_t primitiveCount,
        std::size_t vertexCount, std::size_t vertexSize, DeviceBuffer* vbo, std::size_t vboOffset /* = 0*/,
        DeviceBuffer* ibo /*= nullptr*/, std::size_t iboOffset /*= 0*/)
    {
        auto sbtInstanceOffset = materialSBTMapping[materialInfo.m_materialIdentifier];
        auto blasIndex =
            AddBottomLevelAccelerationStructure(static_cast<std::uint32_t>(m_geometryIndex), sbtInstanceOffset, glm::transpose(transform));

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

        auto [materialType, materialIndex] = AddMaterial(materialInfo);
        AddInstanceInfo(static_cast<std::uint32_t>(vertexSize), static_cast<std::uint32_t>(m_geometryIndex),
                        materialType, materialIndex, transform);
        m_BLAS[blasIndex].AddTriangleGeometry(primitiveCount, vertexCount, vertexSize, false, vertexBufferDeviceAddress,
                                              indexBufferDeviceAddress);
        m_triangleGeometryInfos.emplace_back(m_geometryIndex++, vbo, vboOffset, vertexCount * vertexSize,
                                             ibo ? ibo : vbo, iboOffset, primitiveCount * 3 * sizeof(std::uint32_t));
    }

    void AccelerationStructureGeometry::FinalizeBuffer(const AccelerationStructureBufferInfo& bufferInfo,
                                                       const std::vector<std::uint32_t>& materialSBTMapping)
    {
        std::size_t totalBufferSize = bufferInfo.geometryBufferSize;
        for (std::size_t i = 0; i < m_materials.size(); ++i) {
            m_materialBuffersOffset[i] = m_device->CalculateStorageBufferAlignment(totalBufferSize);
            totalBufferSize = m_materialBuffersOffset[i] + m_materialBuffersRange[i];
        }

        m_bufferIndex =
            m_bufferMemGroup.AddBufferToGroup(fmt::format("ASShaderBuffer:{}", m_name),
                                              vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                  | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                                                  | vk::BufferUsageFlagBits::eStorageBuffer,
                                              totalBufferSize, m_queueFamilyIndices);

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];
            m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, meshInfo.vboOffset, bufferInfo.vertices[i_mesh]);
            m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, meshInfo.iboOffset, bufferInfo.indices[i_mesh]);
        }

        m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, m_instanceBufferOffset, m_instanceInfos);
        for (std::size_t i = 0; i < m_materials.size(); ++i) {
            if (bufferInfo.materials[i].size() == 0) { continue; }
            m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, m_materialBuffersOffset[i], bufferInfo.materials[i]);
        }

        TransferMemGroup();
        // for (std::size_t iType = 0; iType < m_materials.size(); ++iType) {
        //     for (std::size_t i = 0; i < m_materials[iType].size(); ++i) {
        //         for (std::size_t iTex = 0; iTex < m_materials[iType][i].m_materialInfo; ++iTex) {
        //             m_textures[] = &m_materials[iType][i].m_textures[iTex]->GetTexture();
        //         }
        //     }
        //     // if (m_materials[i].m_diffuseTexture) {
        //     //     m_diffuseTextures[m_materialInfos[i].diffuseTextureIndex] =
        //     //         &m_materials[i].m_diffuseTexture->GetTexture();
        //     // }
        //     // if (m_materials[i].m_bumpMap) {
        //     //     m_bumpMaps[m_materialInfos[i].bumpTextureIndex] = &m_materials[i].m_bumpMap->GetTexture();
        //     // }
        // }

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];
            AddMeshNodeGeometry(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform, materialSBTMapping);
        }
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
                m_sbtInstanceOffsets[i],
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
                                                                                   std::uint32_t sbtInstanceOffset,
                                                                                   const glm::mat3x4& transform)
    {
        auto blasIndex = m_BLAS.size();
        m_BLAS.emplace_back(m_device, fmt::format("BLAS:{}-{}", m_name, blasIndex),
                            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        m_bufferIndices.push_back(bufferIndex);
        m_sbtInstanceOffsets.push_back(sbtInstanceOffset);
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
        std::uint32_t bindingIBO, std::uint32_t bindingInstanceBuffer,
        std::uint32_t bindingTextures)
    {
        layout.AddBinding(bindingVBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingIBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingInstanceBuffer, vk::DescriptorType::eStorageBuffer, 1, shaderFlags);
        layout.AddBinding(bindingTextures, vk::DescriptorType::eCombinedImageSampler,
                          static_cast<std::uint32_t>(m_textures.size()), shaderFlags);
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

    void AccelerationStructureGeometry::FillTextureInfo(std::vector<Texture*>& textures)
    {
        for (auto tex : m_textures) { textures.emplace_back(tex); }
    }

    void AccelerationStructureGeometry::CreateResourceUseBarriers(vk::AccessFlags2KHR access,
                                                                  vk::PipelineStageFlags2KHR pipelineStage,
                                                                  vk::ImageLayout newLayout, PipelineBarrier& barrier)
    {
        // TODO: buffer barriers.

        for (auto& matType : m_materials) {
            for (auto& mat : matType) { mat.CreateResourceUseBarriers(access, pipelineStage, newLayout, barrier); }
        }
    }

}
