/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/Texture2D.h"
#include "gfx/meshes/MeshInfo.h"
#include "gfx/vk/QueuedDeviceTransfer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructureGeometry::AccelerationStructureGeometry(LogicalDevice* device, std::string_view name)
        : m_device{device}
        , m_name{name}
        , m_TLAS{device, fmt::format("TLAS:", name), vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace}
        , m_textureSampler{device->GetHandle(), fmt::format("ASSampler:{}", name), vk::UniqueSampler{}}
        , m_memGroup{m_device, fmt::format("ASGeometryMemGroup:{}", name), vk::MemoryPropertyFlags()}
    {
        vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),
                                                vk::Filter::eLinear,
                                                vk::Filter::eLinear,
                                                vk::SamplerMipmapMode::eNearest,
                                                vk::SamplerAddressMode::eRepeat,
                                                vk::SamplerAddressMode::eRepeat,
                                                vk::SamplerAddressMode::eRepeat};
        m_textureSampler.SetHandle(device->GetHandle(), m_device->GetHandle().createSamplerUnique(samplerCreateInfo));
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform)
    {
        m_meshGeometryInfos.emplace_back(m_geometryIndex++, &mesh, transform);
    }

    void AccelerationStructureGeometry::FinalizeGeometry()
    {
        // std::uint32_t nextDiffuseTextureIndex = 0;
        // std::uint32_t nextBumpTextureIndex = 0;
        for (auto& meshInfo : m_meshGeometryInfos) {
            const std::size_t bufferSize = meshInfo.iboOffset + meshInfo.iboRange;
            meshInfo.bufferIndex = m_memGroup.AddBufferToGroup(
                fmt::format("ASMeshBuffer:{}-{}", m_name, meshInfo.index),
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                    | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                bufferSize, std::vector<std::uint32_t>{{0, 1}});
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.vboOffset, meshInfo.vertices);
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.iboOffset, meshInfo.indices);

            auto materialOffset = m_materials.size();
            for (const auto& mat : meshInfo.mesh->GetMaterials()) {
                m_materials.emplace_back(&mat, m_device, m_memGroup, std::vector<std::uint32_t>{{0, 1}});
                // auto diffuseIndex = m_materials.back().m_diffuseTexture ? nextDiffuseTextureIndex++ : -1;
                // auto bumpIndex = m_materials.back().m_bumpMap ? nextBumpTextureIndex++ : -1;
                // m_materialTextureIndices.emplace_back(diffuseIndex, bumpIndex);
            }
            AddMeshNodeInstance(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform,
                                static_cast<std::uint32_t>(materialOffset));
        }

        AddInstanceBufferAndTransferMemGroup();

        for (auto& meshInfo : m_meshGeometryInfos) {
            // free some memory.
            meshInfo.indices.clear();
            meshInfo.vertices.clear();

            AddMeshNodeGeometry(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform);
        }
    }

    void AccelerationStructureGeometry::AddInstanceBufferAndTransferMemGroup()
    {
        m_instanceBufferIndex =
            m_memGroup.AddBufferToGroup(fmt::format("ASInstanceBuffer:{}", m_name), vk::BufferUsageFlagBits::eShaderDeviceAddress
                                            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                                            | vk::BufferUsageFlagBits::eStorageBuffer,
                                        m_instanceInfos, std::vector<std::uint32_t>{{0, 1}});

        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, m_device->GetQueue(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
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
        vk::DeviceOrHostAddressConstKHR bufferDeviceAddress =
            m_memGroup.GetBuffer(mesh.bufferIndex)->GetDeviceAddressConst();
        vk::DeviceOrHostAddressConstKHR vboDeviceAddress = bufferDeviceAddress.deviceAddress + mesh.vboOffset;
        vk::DeviceOrHostAddressConstKHR iboDeviceAddress = bufferDeviceAddress.deviceAddress + mesh.iboOffset;

        vk::DeviceOrHostAddressConstKHR indexBufferAddress{iboDeviceAddress.deviceAddress
                                                           + subMesh.GetIndexOffset() * sizeof(std::uint32_t)};

        auto blasIndex =
            AddBottomLevelAccelerationStructure(fmt::format("BLAS:{}-{}", m_name, m_BLAS.size()),
                                                static_cast<std::uint32_t>(mesh.index), glm::transpose(transform));
        m_BLAS[blasIndex].AddTriangleGeometry(subMesh.GetNumberOfTriangles(), subMesh.GetNumberOfIndices(),
                                              mesh.vertexSize, vboDeviceAddress, indexBufferAddress);
    }

    void AccelerationStructureGeometry::AddInstanceInfo(std::uint32_t vertexSize, std::uint32_t bufferIndex,
                                                        std::uint32_t materialIndex, const glm::mat4& transform,
                                                        std::uint32_t indexOffset /*= 0*/)
    {
        auto& instanceInfo =
            m_instanceInfos.emplace_back(vertexSize, bufferIndex, materialIndex, indexOffset);
        // instanceInfo.diffuseTextureIndex = m_materialTextureIndices[materialIndex].first;
        // instanceInfo.bumpTextureIndex = m_materialTextureIndices[materialIndex].second;
        instanceInfo.transform = transform;
        instanceInfo.transformInverseTranspose =
            glm::mat4(glm::mat3(glm::transpose(glm::inverse(instanceInfo.transform))));
    }

    void AccelerationStructureGeometry::AddTriangleGeometry(const glm::mat4& transform, std::size_t primitiveCount,
                                                            std::size_t vertexCount, std::size_t vertexSize,
                                                            const DeviceBuffer* vbo, std::size_t vboOffset /* = 0*/,
                                                            const DeviceBuffer* ibo /*= nullptr*/,
                                                            std::size_t iboOffset /*= 0*/)
    {
        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress =
            vbo->GetDeviceAddressConst().deviceAddress + vboOffset;
        auto vboBuffer = vbo->GetHandle();
        auto iboBuffer = vboBuffer;
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress = vertexBufferDeviceAddress;
        if (ibo != nullptr) {
            iboBuffer = ibo->GetHandle();
            indexBufferDeviceAddress = ibo->GetDeviceAddressConst().deviceAddress + iboOffset;
        } else {
            if (iboOffset == 0) {
                iboOffset = m_device->CalculateStorageBufferAlignment(vboOffset + vertexCount * vertexSize);
            }
            indexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress + iboOffset;
        }

        auto blasIndex =
            AddBottomLevelAccelerationStructure(fmt::format("BLAS:{}-{}", m_name, m_BLAS.size()),
                                                static_cast<std::uint32_t>(m_geometryIndex), glm::transpose(transform));
        AddInstanceInfo(static_cast<std::uint32_t>(vertexSize), static_cast<std::uint32_t>(m_geometryIndex),
                        static_cast<std::uint32_t>(-1), transform);
        m_BLAS[blasIndex].AddTriangleGeometry(primitiveCount, vertexCount, vertexSize, vertexBufferDeviceAddress,
                                              indexBufferDeviceAddress);
        m_triangleGeometryInfos.emplace_back(m_geometryIndex++, vboBuffer, vboOffset, vertexCount * vertexSize,
                                             iboBuffer, iboOffset, primitiveCount * 3 * sizeof(std::uint32_t));
    }

    void AccelerationStructureGeometry::BuildAccelerationStructure()
    {
        // TODO: a way to build all BLAS at once would be good. [11/22/2020 Sebastian Maisch]
        for (std::size_t i = 0; i < m_BLAS.size(); ++i) {
            m_BLAS[i].BuildAccelerationStructure();

            vk::AccelerationStructureInstanceKHR blasInstance{
                vk::TransformMatrixKHR{},
                m_bufferIndices[i],
                0xFF,
                0,
                vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
                m_BLAS[i].GetAddressHandle()};
            memcpy(&blasInstance.transform, &m_BLASTransforms[i], sizeof(glm::mat3x4));
            m_TLAS.AddBottomLevelAccelerationStructureInstance(blasInstance);
        }

        m_TLAS.BuildAccelerationStructure();
    }

    std::size_t AccelerationStructureGeometry::AddBottomLevelAccelerationStructure(std::string_view name, std::uint32_t bufferIndex,
                                                                                   const glm::mat3x4& transform)
    {
        auto result = m_BLAS.size();
        m_BLAS.emplace_back(m_device, name, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        m_BLASTransforms.emplace_back(transform);
        m_bufferIndices.push_back(bufferIndex);
        return result;
    }

    void AccelerationStructureGeometry::AddDescriptorLayoutBindingAS(DescriptorSetLayout& layout,
                                                                     vk::ShaderStageFlags shaderFlags,
                                                                     std::uint32_t bindingAS)
    {
        layout.AddBinding(bindingAS, vk::DescriptorType::eAccelerationStructureKHR, 1, shaderFlags);
    }

    void AccelerationStructureGeometry::AddDescriptorLayoutBindingBuffers(
        DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags, std::uint32_t bindingVBO,
        std::uint32_t bindingIBO, std::uint32_t bindingInstanceBuffer, std::uint32_t bindingDiffuseTexture,
        std::uint32_t bindingBumpTexture)
    {
        layout.AddBinding(bindingVBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingIBO, vk::DescriptorType::eStorageBuffer,
                          static_cast<std::uint32_t>(m_triangleGeometryInfos.size() + m_meshGeometryInfos.size()),
                          shaderFlags);
        layout.AddBinding(bindingInstanceBuffer, vk::DescriptorType::eStorageBuffer, 1, shaderFlags);
        // TODO: set correct count [10/4/2021 Sebastian Maisch]
        layout.AddBinding(bindingDiffuseTexture, vk::DescriptorType::eCombinedImageSampler,
                          static_cast<std::uint32_t>(m_materials.size()), shaderFlags);
        layout.AddBinding(bindingBumpTexture, vk::DescriptorType::eCombinedImageSampler,
                          static_cast<std::uint32_t>(m_materials.size()), shaderFlags);
    }

    void AccelerationStructureGeometry::FillDescriptorAccelerationStructureInfo(
        vk::WriteDescriptorSetAccelerationStructureKHR& descInfo) const
    {
        descInfo.setAccelerationStructureCount(1);
        descInfo.setPAccelerationStructures(m_TLAS.GetHandlePtr());
    }

    void AccelerationStructureGeometry::FillDescriptorBuffersInfo(
        std::vector<vk::DescriptorBufferInfo>& vboBufferInfos, std::vector<vk::DescriptorBufferInfo>& iboBufferInfos,
        vk::DescriptorBufferInfo& instanceBufferInfo, std::vector<vk::DescriptorImageInfo>& diffuseTextureInfos,
        std::vector<vk::DescriptorImageInfo>& bumpTextureInfos) const
    {
        auto vboIOffset = vboBufferInfos.size();
        auto iboIOffset = iboBufferInfos.size();
        vboBufferInfos.resize(vboIOffset + m_triangleGeometryInfos.size() + m_meshGeometryInfos.size());
        iboBufferInfos.resize(iboIOffset + m_triangleGeometryInfos.size() + m_meshGeometryInfos.size());

        for (const auto& triangleGeometry : m_triangleGeometryInfos) {
            vboBufferInfos[vboIOffset + triangleGeometry.index] = vk::DescriptorBufferInfo{
                triangleGeometry.vboBuffer, triangleGeometry.vboOffset, triangleGeometry.vboRange};
            iboBufferInfos[iboIOffset + triangleGeometry.index] = vk::DescriptorBufferInfo{
                triangleGeometry.iboBuffer, triangleGeometry.iboOffset, triangleGeometry.iboRange};
        }

        for (const auto& meshGeometry : m_meshGeometryInfos) {
            vboBufferInfos[vboIOffset + meshGeometry.index] =
                vk::DescriptorBufferInfo{m_memGroup.GetBuffer(meshGeometry.bufferIndex)->GetHandle(),
                                         meshGeometry.vboOffset, meshGeometry.vboRange};
            iboBufferInfos[iboIOffset + meshGeometry.index] =
                vk::DescriptorBufferInfo{m_memGroup.GetBuffer(meshGeometry.bufferIndex)->GetHandle(),
                                         meshGeometry.iboOffset, meshGeometry.iboRange};
        }

        instanceBufferInfo.setBuffer(m_memGroup.GetBuffer(m_instanceBufferIndex)->GetHandle());
        instanceBufferInfo.setOffset(0);
        instanceBufferInfo.setRange(VK_WHOLE_SIZE);

        for (const auto& mat : m_materials) {
            if (mat.m_diffuseTexture) {
                diffuseTextureInfos.emplace_back(m_textureSampler.GetHandle(), mat.m_diffuseTexture->GetTexture().GetImageView().GetHandle(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal);
            } else {
                diffuseTextureInfos.emplace_back(m_textureSampler.GetHandle(), nullptr,
                                                 // m_device->GetDummyTexture()->GetTexture().GetImageView().GetHandle(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal);
            }

            if (mat.m_bumpMap) {
                bumpTextureInfos.emplace_back(m_textureSampler.GetHandle(),
                                              mat.m_bumpMap->GetTexture().GetImageView().GetHandle(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal);
            } else {
                bumpTextureInfos.emplace_back(m_textureSampler.GetHandle(), nullptr,
                                              // m_device->GetDummyTexture()->GetTexture().GetImageView().GetHandle(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal);
            }
        }
    }

}
