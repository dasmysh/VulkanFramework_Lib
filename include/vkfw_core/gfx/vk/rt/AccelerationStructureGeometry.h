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

        struct AccelerationStructureBufferInfo
        {
            std::size_t geometryBufferSize = 0;
            std::vector<std::vector<std::uint32_t>> indices;
            std::vector<std::vector<std::uint8_t>> vertices;
            std::vector<std::vector<std::uint8_t>> materials;
            std::vector<std::size_t> materialBufferOffsets;
        };

        template<Vertex VertexType> void FinalizeGeometry(AccelerationStructureBufferInfo& bufferInfo);
        template<vkfw_core::Material MaterialType> void FinalizeMaterial(AccelerationStructureBufferInfo& bufferInfo);
        void FinalizeBuffer(const AccelerationStructureBufferInfo& bufferInfo);

        void BuildAccelerationStructure();

        void AddDescriptorLayoutBindingAS(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                          std::uint32_t bindingAS);
        void AddDescriptorLayoutBindingBuffers(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                               std::uint32_t bindingVBO, std::uint32_t bindingIBO,
                                               std::uint32_t bindingInstanceBuffer, std::uint32_t bindingMaterialBuffer,
                                               std::uint32_t bindingTextures);
        [[nodiscard]] vk::AccelerationStructureKHR
        GetTopLevelAccelerationStructure(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                         PipelineBarrier& barrier) const;
        void FillGeometryInfo(std::vector<BufferRange>& vbos, std::vector<BufferRange>& ibos,
                              BufferRange& instanceBuffer);
        template<vkfw_core::Material MaterialType> void FillMaterialInfo(BufferRange& materialBuffer);
        void FillTextureInfo(std::vector<Texture*>& textures);
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

        void TransferMemGroup();
        [[nodiscard]] std::size_t AddBottomLevelAccelerationStructure(std::uint32_t bufferIndex,
                                                                      const glm::mat3x4& transform);
        void AddInstanceInfo(std::uint32_t vertexSize, std::uint32_t bufferIndex, std::uint32_t materialType,
                             std::uint32_t materialIndex, const glm::mat4& transform, std::uint32_t indexOffset = 0);
        void AddMeshNodeInstance(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform,
                                 const std::vector<std::pair<std::uint32_t, std::uint32_t>>& materialMapping);
        void AddMeshNodeGeometry(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform);
        void AddSubMeshGeometry(const MeshGeometryInfo& mesh, const SubMesh& subMesh, const glm::mat4& transform);

        std::pair<std::uint32_t, std::uint32_t> AddMaterial(const MaterialInfo& materialInfo);

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
        std::vector<std::vector<Material>> m_materials;
        // std::vector<std::vector<std::uint8_t>> m_materialInfos;
        std::vector<Texture*> m_textures;
        /** The offset and range of the material buffer. */
        std::vector<std::size_t> m_materialBuffersOffset;
        std::vector<std::size_t> m_materialBuffersRange;
    };

    template<Vertex VertexType>
    void AccelerationStructureGeometry::FinalizeGeometry(AccelerationStructureBufferInfo& bufferInfo)
    {
        bufferInfo.indices.resize(m_meshGeometryInfos.size());
        bufferInfo.vertices.resize(m_meshGeometryInfos.size());

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            std::vector<VertexType> vertices;
            auto& meshInfo = m_meshGeometryInfos[i_mesh];

            meshInfo.mesh->GetVertices<VertexType>(vertices);
            bufferInfo.indices[i_mesh] = meshInfo.mesh->GetIndices();
            bufferInfo.vertices[i_mesh].resize(byteSizeOf(vertices));
            memcpy(bufferInfo.vertices[i_mesh].data(), vertices.data(), bufferInfo.vertices[i_mesh].size());

            meshInfo.vertexSize = sizeof(VertexType);
            meshInfo.vboRange = byteSizeOf(vertices);
            meshInfo.vboOffset = m_device->CalculateStorageBufferAlignment(bufferInfo.geometryBufferSize);
            meshInfo.iboRange = byteSizeOf(bufferInfo.indices[i_mesh]);
            meshInfo.iboOffset = m_device->CalculateStorageBufferAlignment(meshInfo.vboOffset + meshInfo.vboRange);
            bufferInfo.geometryBufferSize = meshInfo.iboOffset + meshInfo.iboRange;

            std::vector<std::pair<std::uint32_t, std::uint32_t>> materialMapping;
            for (const auto& mat : meshInfo.mesh->GetMaterials()) { materialMapping.emplace_back(AddMaterial(*mat)); }
            AddMeshNodeInstance(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform, materialMapping);
        }
        m_instanceBufferRange = byteSizeOf(m_instanceInfos);
        m_instanceBufferOffset = m_device->CalculateStorageBufferAlignment(bufferInfo.geometryBufferSize);
        bufferInfo.geometryBufferSize = m_instanceBufferOffset + m_instanceBufferRange;
    }

    template<vkfw_core::Material MaterialType>
    void AccelerationStructureGeometry::FinalizeMaterial(AccelerationStructureBufferInfo& bufferInfo)
    {
        if (bufferInfo.materials.size() <= MaterialType::MATERIAL_ID) {
            bufferInfo.materials.resize(MaterialType::MATERIAL_ID + 1);
            m_materialBuffersRange.resize(MaterialType::MATERIAL_ID + 1, 0);
            m_materialBuffersOffset.resize(MaterialType::MATERIAL_ID + 1, 0);
        }

        constexpr std::size_t iType = MaterialType::MATERIAL_ID;
        const std::size_t materialGPUSize = MaterialType::GetGPUSize();
        bufferInfo.materials[iType].resize(m_materials[iType].size() * materialGPUSize);
        std::size_t materialStart = 0;
        for (std::size_t i = 0; i < m_materials[iType].size(); ++i) {
            auto texturesOffset = m_textures.size();
            auto materialBuffer = std::span{&bufferInfo.materials[iType][materialStart], materialGPUSize};
            MaterialType::FillGPUInfo(*static_cast<const MaterialType*>(m_materials[iType][i].m_materialInfo),
                                      materialBuffer, static_cast<std::uint32_t>(texturesOffset));


            m_textures.resize(texturesOffset + m_materials[iType][i].m_materialInfo->GetTextureCount());
            for (std::size_t iTex = 0; iTex < m_materials[iType][i].m_materialInfo->GetTextureCount(); ++iTex) {
                m_textures[texturesOffset + iTex] = &m_materials[iType][i].m_textures[iTex]->GetTexture();
            }

            //
            //     if (m_materials[iType][i].m_diffuseTexture) {
            //         m_materialInfos[iType][i].diffuseTextureIndex =
            //             static_cast<std::uint32_t>(m_diffuseTextures.size());
            //         m_diffuseTextures.emplace_back(nullptr);
            //     }
            //     if (m_materials[iType][i].m_bumpMap) {
            //         m_materialInfos[iType][i].bumpTextureIndex = static_cast<std::uint32_t>(m_bumpMaps.size());
            //         m_bumpMaps.emplace_back(nullptr);
            //     }
            //     m_materialInfos[iType][i].diffuseColor =
            //         glm::vec4{m_materials[iType][i].m_materialInfo->m_diffuse, 1.0f};
        }

        m_materialBuffersRange[iType] = byteSizeOf(bufferInfo.materials[iType]);
    }

    template<vkfw_core::Material MaterialType>
    void AccelerationStructureGeometry::FillMaterialInfo(BufferRange& materialBuffer)
    {
        materialBuffer.m_buffer = m_bufferMemGroup.GetBuffer(m_bufferIndex);
        materialBuffer.m_offset = m_materialBuffersOffset[MaterialType::MATERIAL_ID];
        materialBuffer.m_range = m_materialBuffersRange[MaterialType::MATERIAL_ID];
    }
}
