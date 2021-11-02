/**
 * @file   AccelerationStructureGeometry.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Declaration of the acceleration structure geometry class.
 */

#pragma once

#include "main.h"
#include "gfx/meshes/MeshInfo.h"
#include "gfx/vk/rt/BottomLevelAccelerationStructure.h"
#include "gfx/vk/rt/TopLevelAccelerationStructure.h"
#include "gfx/vk/rt/AccelerationStructure.h"
#include "gfx/Material.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "core/concepts.h"
#include <glm/mat4x4.hpp>
#include "rt/ray_tracing_host_interface.h"
#include "gfx/Texture2D.h"

namespace vkfw_core::gfx {
    class DescriptorSetLayout;
    class LogicalDevice;
    class DeviceBuffer;
    class MeshInfo;
    class SceneMeshNode;
    class SubMesh;
    class DeviceBuffer;
    class PipelineBarrier;
    struct BufferRange;
    struct AccelerationStructureInfo;
}

namespace vkfw_core::gfx::rt {

    class AccelerationStructureGeometry
    {
    public:
        AccelerationStructureGeometry(LogicalDevice* device, std::string_view name,
                                      const std::vector<std::uint32_t>& queueFamilyIndices);
        ~AccelerationStructureGeometry();

        void AddTriangleGeometry(const glm::mat4& transform, const MaterialInfo& materialInfo,
                                 std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
                                 DeviceBuffer* vbo, std::size_t vboOffset = 0, DeviceBuffer* ibo = nullptr,
                                 std::size_t iboOffset = 0);

        void AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform);
        template<Vertex VertexType> void FinalizeGeometry();

        void BuildAccelerationStructure();

        void AddDescriptorLayoutBindingAS(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                          std::uint32_t bindingAS);
        void AddDescriptorLayoutBindingBuffers(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                               std::uint32_t bindingVBO, std::uint32_t bindingIBO,
                                               std::uint32_t bindingInstanceBuffer, std::uint32_t bindingMaterialBuffer,
                                               std::uint32_t bindingDiffuseTexture, std::uint32_t bindingBumpTexture);
        [[nodiscard]] vk::AccelerationStructureKHR
        GetTopLevelAccelerationStructure(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                         PipelineBarrier& barrier) const;
        void FillGeometryInfo(std::vector<BufferRange>& vbos, std::vector<BufferRange>& ibos,
                              BufferRange& instanceBuffer);
        void FillMaterialInfo(BufferRange& materialBuffer, std::vector<Texture*>& diffuseTextures,
                              std::vector<Texture*>& bumpMaps);
        void CreateResourceUseBarriers(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStage,
                                       vk::ImageLayout newLayout, PipelineBarrier& barrier);

    private:
        struct MeshGeometryInfo
        {
            std::size_t index = 0;
            const MeshInfo* mesh = nullptr;
            glm::mat4 transform = glm::mat4{1.0f};
            std::size_t vertexSize = 0;
            std::size_t vboOffset = 0;
            std::size_t vboRange = 0;
            std::size_t iboOffset = 0;
            std::size_t iboRange = 0;
        };

        struct TriangleGeometryInfo
        {
            std::size_t index = 0;
            Buffer* vboBuffer;
            std::size_t vboOffset = 0;
            std::size_t vboRange = 0;
            Buffer* iboBuffer;
            std::size_t iboOffset = 0;
            std::size_t iboRange = 0;
        };

        void AddInstanceBufferAndTransferMemGroup();
        [[nodiscard]] std::size_t AddBottomLevelAccelerationStructure(std::uint32_t bufferIndex,
                                                                      const glm::mat3x4& transform);
        void AddInstanceInfo(std::uint32_t vertexSize, std::uint32_t bufferIndex, std::uint32_t materialIndex,
                             const glm::mat4& transform, std::uint32_t indexOffset = 0);
        void AddMeshNodeInstance(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform,
                                 std::uint32_t materialOffset);
        void AddMeshNodeGeometry(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform);
        void AddSubMeshGeometry(const MeshGeometryInfo& mesh, const SubMesh& subMesh, const glm::mat4& transform);

        /** The device to create the acceleration structures in. */
        LogicalDevice* m_device;
        /** The structures name. */
        std::string m_name;
        /** The queue family indices used. */
        std::vector<std::uint32_t> m_queueFamilyIndices;

        /** The bottom level acceleration structure for the scene. */
        std::vector<BottomLevelAccelerationStructure> m_BLAS;
        /** The transformations for the bottom level acceleration structures. */
        std::vector<glm::mat3x4> m_BLASTransforms;
        /** The top level acceleration structure for the scene. */
        TopLevelAccelerationStructure m_TLAS;
        /** The sampler for the materials textures. */
        // Sampler m_textureSampler;

        /** Holds the memory for geometry and instance buffers. */
        MemoryGroup m_bufferMemGroup;
        /** Holds the memory for textures. */
        MemoryGroup m_textureMemGroup;
        /** Holds the information for the the meshes vertex and index buffers. */
        std::vector<MeshGeometryInfo> m_meshGeometryInfos;
        /** The information about triangle geometries. */
        std::vector<TriangleGeometryInfo> m_triangleGeometryInfos;
        /** An internal counter for the current geometry index. */
        std::size_t m_geometryIndex = 0;
        /** Contains for each geometry index an index to a vbo/ibo pair. */
        std::vector<std::uint32_t> m_bufferIndices;
        std::uint32_t m_bufferIndex = MemoryGroup::INVALID_INDEX;
        /** Contains further information about each instance like material indices. */
        std::vector<InstanceDesc> m_instanceInfos;
        /** The offset and range of the instances buffer. */
        std::size_t m_instanceBufferOffset = 0;
        std::size_t m_instanceBufferRange = 0;
        /** Holds the materials. */
        std::vector<Material> m_materials;
        std::vector<MaterialDesc> m_materialInfos;
        std::vector<Texture*> m_diffuseTextures;
        std::vector<Texture*> m_bumpMaps;
        /** The offset and range of the material buffer. */
        std::size_t m_materialBufferOffset = 0;
        std::size_t m_materialBufferRange = 0;
    };

    template<Vertex VertexType>
    void AccelerationStructureGeometry::FinalizeGeometry()
    {
        std::vector<std::vector<VertexType>> vertices{m_meshGeometryInfos.size()};
        std::vector<std::vector<std::uint32_t>> indices{m_meshGeometryInfos.size()};
        std::size_t totalBufferSize = 0;
        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            auto& meshInfo = m_meshGeometryInfos[i_mesh];
            meshInfo.mesh->GetVertices(vertices[i_mesh]);
            indices[i_mesh] = meshInfo.mesh->GetIndices();

            meshInfo.vertexSize = sizeof(VertexType);
            meshInfo.vboRange = byteSizeOf(vertices[i_mesh]);
            meshInfo.vboOffset = m_device->CalculateStorageBufferAlignment(totalBufferSize);
            meshInfo.iboRange = byteSizeOf(indices[i_mesh]);
            meshInfo.iboOffset = m_device->CalculateStorageBufferAlignment(meshInfo.vboOffset + meshInfo.vboRange);
            totalBufferSize = meshInfo.iboOffset + meshInfo.iboRange;

            auto materialOffset = m_materials.size();
            for (const auto& mat : meshInfo.mesh->GetMaterials()) {
                m_materials.emplace_back(&mat, m_device, m_textureMemGroup, m_queueFamilyIndices);
            }
            AddMeshNodeInstance(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform,
                                static_cast<std::uint32_t>(materialOffset));
        }
        m_instanceBufferRange = byteSizeOf(m_instanceInfos);
        m_instanceBufferOffset = m_device->CalculateStorageBufferAlignment(totalBufferSize);
        totalBufferSize = m_instanceBufferOffset + m_instanceBufferRange;

        m_materialInfos.resize(m_materials.size(),
                               MaterialDesc{INVALID_TEXTURE_INDEX, INVALID_TEXTURE_INDEX, glm::vec4{1.0f}});
        for (std::size_t i = 0; i < m_materials.size(); ++i) {
            if (m_materials[i].m_diffuseTexture) {
                m_materialInfos[i].diffuseTextureIndex = static_cast<std::uint32_t>(m_diffuseTextures.size());
                m_diffuseTextures.emplace_back(nullptr);
            }
            if (m_materials[i].m_bumpMap) {
                m_materialInfos[i].bumpTextureIndex = static_cast<std::uint32_t>(m_bumpMaps.size());
                m_bumpMaps.emplace_back(nullptr);
            }
            m_materialInfos[i].diffuseColor = glm::vec4{m_materials[i].m_materialInfo->m_diffuse, 1.0f};
        }
        m_materialBufferRange = byteSizeOf(m_materialInfos);
        m_materialBufferOffset = m_device->CalculateStorageBufferAlignment(totalBufferSize);
        totalBufferSize = m_materialBufferOffset + m_materialBufferRange;

        m_bufferIndex =
            m_bufferMemGroup.AddBufferToGroup(fmt::format("ASShaderBuffer:{}", m_name),
                                              vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                  | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                                                  | vk::BufferUsageFlagBits::eStorageBuffer,
                                              totalBufferSize, m_queueFamilyIndices);


        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];
            m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, meshInfo.vboOffset, vertices[i_mesh]);
            m_bufferMemGroup.AddDataToBufferInGroup(m_bufferIndex, meshInfo.iboOffset, indices[i_mesh]);
        }

        AddInstanceBufferAndTransferMemGroup();
        for (std::size_t i = 0; i < m_materials.size(); ++i) {
            if (m_materials[i].m_diffuseTexture) {
                m_diffuseTextures[m_materialInfos[i].diffuseTextureIndex] =
                    &m_materials[i].m_diffuseTexture->GetTexture();
            }
            if (m_materials[i].m_bumpMap) {
                m_bumpMaps[m_materialInfos[i].bumpTextureIndex] = &m_materials[i].m_bumpMap->GetTexture();
            }
        }

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];
            AddMeshNodeGeometry(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform);
        }
    }
}
