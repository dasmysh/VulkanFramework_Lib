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
#include "gfx/vk/memory/MemoryGroup.h"
#include "core/concepts.h"
#include <glm/mat4x4.hpp>

namespace vkfw_core::gfx {
    class DescriptorSetLayout;
    class LogicalDevice;
    class DeviceBuffer;
    class MeshInfo;
    class SceneMeshNode;
    class SubMesh;
    class DeviceBuffer;
}

namespace vkfw_core::gfx::rt {

    class AccelerationStructureGeometry
    {
    public:
        AccelerationStructureGeometry(LogicalDevice* device);
        ~AccelerationStructureGeometry();

        // there are 2 options to add meshes:
        // 1: each mesh can have a different vertex type.
        template<Vertex VertexType>
        std::size_t AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform);
        void FinalizeGeometry();

        // 2: all meshes use the same vertex type.
        void AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform);
        template<Vertex VertexType> void FinalizeGeometry();

        // [[nodiscard]] BottomLevelAccelerationStructure& GetBottomLevelAccelerationStructure(std::size_t index)
        // {
        //     return m_BLAS[index];
        // }
        // [[nodiscard]] glm::mat3x4& GetBottomLevelAccelerationStructureTransform(std::size_t index)
        // {
        //     return m_BLASTransforms[index];
        // }
        void AddTriangleGeometry(const glm::mat4& transform, std::size_t primitiveCount, std::size_t vertexCount,
                                 std::size_t vertexSize, const DeviceBuffer* vbo, std::size_t vboOffset = 0,
                                 const DeviceBuffer* ibo = nullptr, std::size_t iboOffset = 0);

        void BuildAccelerationStructure();

        void AddDescriptorLayoutBindingAS(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                          std::uint32_t bindingAS);
        void AddDescriptorLayoutBindingBuffers(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                               std::uint32_t bindingVBO, std::uint32_t bindingIBO,
                                               std::uint32_t bindingInstanceBuffer);
        void FillDescriptorAccelerationStructureInfo(vk::WriteDescriptorSetAccelerationStructureKHR& descInfo) const;
        void FillDescriptorBuffersInfo(std::vector<vk::DescriptorBufferInfo>& vboBufferInfos,
                                       std::vector<vk::DescriptorBufferInfo>& iboBufferInfos,
                                       vk::DescriptorBufferInfo& instanceBufferInfo) const;

    private:
        struct MeshGeometryInfo
        {
            std::size_t index = 0;
            const MeshInfo* mesh = nullptr;
            glm::mat4 transform = glm::mat4{1.0f};
            unsigned int bufferIndex = DeviceMemoryGroup::INVALID_INDEX;
            std::size_t vertexSize = 0;
            std::size_t vboOffset = 0;
            std::size_t vboRange = 0;
            std::size_t iboOffset = 0;
            std::size_t iboRange = 0;
            std::vector<std::uint8_t> vertices;
            std::vector<std::uint32_t> indices;
        };

        struct TriangleGeometryInfo
        {
            std::size_t index = 0;
            vk::Buffer vboBuffer;
            std::size_t vboOffset = 0;
            std::size_t vboRange = 0;
            vk::Buffer iboBuffer;
            std::size_t iboOffset = 0;
            std::size_t iboRange = 0;
        };

        struct InstanceInfo
        {
            std::uint32_t bufferIndex = 0;
            std::uint32_t materialIndex = 0;
            std::uint32_t textureIndex = 0;
            std::uint32_t indexOffset = 0;
            glm::mat4 transform = glm::mat4{1.0f};
            glm::mat4 transformInverseTranspose = glm::mat4{1.0f};
        };

        void AddInstanceBufferAndTransferMemGroup();
        [[nodiscard]] std::size_t AddBottomLevelAccelerationStructure(std::uint32_t bufferIndex,
                                                                      const glm::mat3x4& transform);
        void AddInstanceInfo(std::uint32_t bufferIndex, const glm::mat4& transform, std::uint32_t indexOffset = 0);
        void AddMeshNodeInstance(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform);
        void AddMeshNodeGeometry(const MeshGeometryInfo& mesh, const SceneMeshNode* node, const glm::mat4& transform);
        void AddSubMeshGeometry(const MeshGeometryInfo& mesh, const SubMesh& subMesh, const glm::mat4& transform);

        /** The device to create the acceleration structures in. */
        LogicalDevice* m_device;

        /** The bottom level acceleration structure for the scene. */
        std::vector<BottomLevelAccelerationStructure> m_BLAS;
        /** The transformations for the bottom level acceleration structures. */
        std::vector<glm::mat3x4> m_BLASTransforms;
        /** The top level acceleration structure for the scene. */
        TopLevelAccelerationStructure m_TLAS;

        /** Holds the memory for geometry if needed. */
        MemoryGroup m_memGroup;
        /** Holds the information for the the meshes vertex and index buffers. */
        std::vector<MeshGeometryInfo> m_meshGeometryInfos;
        /** The information about triangle geometries. */
        std::vector<TriangleGeometryInfo> m_triangleGeometryInfos;
        /** An internal counter for the current geometry index. */
        std::size_t m_geometryIndex = 0;
        /** Contains for each geometry index an index to a vbo/ibo pair. */
        std::vector<std::uint32_t> m_bufferIndices;
        /** Contains further information about each instance like material indices. */
        std::vector<InstanceInfo> m_instanceInfos;
        /** The index to the instances buffer. */
        std::uint32_t m_instanceBufferIndex = 0;
    };

    template<Vertex VertexType> void AccelerationStructureGeometry::FinalizeGeometry()
    {
        std::vector<std::vector<VertexType>> vertices{m_meshGeometryInfos.size()};
        std::vector<std::vector<std::uint32_t>> indices{m_meshGeometryInfos.size()};
        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            auto& meshInfo = m_meshGeometryInfos[i_mesh];
            meshInfo.mesh->GetVertices(vertices[i_mesh]);
            indices[i_mesh] = meshInfo.mesh->GetIndices();

            meshInfo.vboRange = byteSizeOf(vertices[i_mesh]);
            meshInfo.vboOffset = 0;
            meshInfo.iboRange = byteSizeOf(indices[i_mesh]);
            meshInfo.iboOffset = m_device->CalculateStorageBufferAlignment(meshInfo.vboRange);
            meshInfo.vertexSize = sizeof(VertexType);

            const std::size_t bufferSize = meshInfo.iboOffset + meshInfo.iboRange;

            meshInfo.bufferIndex = m_memGroup.AddBufferToGroup(
                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                    | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                bufferSize, std::vector<std::uint32_t>{{0, 1}});
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.vboOffset, vertices[i_mesh]);
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.iboOffset, indices[i_mesh]);

            AddMeshNodeInstance(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform);
        }

        AddInstanceBufferAndTransferMemGroup();

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];
            AddMeshNodeGeometry(meshInfo, meshInfo.mesh->GetRootNode(), meshInfo.transform);
        }
    }

    template<Vertex VertexType>
    std::size_t AccelerationStructureGeometry::AddMeshGeometry(const MeshInfo& mesh, const glm::mat4& transform)
    {
        auto meshIndex = m_meshGeometryInfos.size();

        auto& meshInfo = m_meshGeometryInfos.emplace_back(m_geometryIndex++, &mesh, transform);
        meshInfo.indices = mesh.GetIndices();

        std::vector<VertexType> vertices;
        mesh.GetVertices(vertices);

        meshInfo.vertexSize = sizeof(VertexType);
        meshInfo.vboRange = byteSizeOf(vertices);
        meshInfo.vboOffset = 0;
        meshInfo.iboRange = byteSizeOf(meshInfo.indices);
        meshInfo.iboOffset = m_device->CalculateStorageBufferAlignment(meshInfo.vboRange);
        meshInfo.vertices.resize(byteSizeOf(vertices));
        memcpy(meshInfo.vertices.data(), vertices.data(), byteSizeOf(vertices));

        return meshIndex;
    }
}
