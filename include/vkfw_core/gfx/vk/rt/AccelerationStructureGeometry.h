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
}

namespace vkfw_core::gfx::rt {

    class AccelerationStructureGeometry
    {
    public:
        AccelerationStructureGeometry(vkfw_core::gfx::LogicalDevice* device);
        ~AccelerationStructureGeometry();

        // there are 2 options to add meshes:
        // 1: each mesh can have a different vertex type.
        template<Vertex VertexType>
        std::size_t AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh, const glm::mat4& transform);
        void FinalizeMeshGeometry();

        // 2: all meshes use the same vertex type.
        void AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh, const glm::mat4& transform);
        template<Vertex VertexType> void FinalizeMeshGeometry();

        void AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh, const glm::mat4& transform, std::size_t vertexSize,
                             vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                             vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);

        std::size_t AddBottomLevelAccelerationStructure(const glm::mat3x4& transform);
        BottomLevelAccelerationStructure& GetBottomLevelAccelerationStructure(std::size_t index) { return m_BLAS[index]; }
        glm::mat3x4& GetBottomLevelAccelerationStructureTransform(std::size_t index) { return m_BLASTransforms[index]; }

        void BuildAccelerationStructure();

        static void AddDescriptorLayoutBinding(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                               std::uint32_t binding = 0);
        void FillDescriptorAccelerationStructureInfo(vk::WriteDescriptorSetAccelerationStructureKHR& descInfo) const;
        void FillDescriptorBufferInfo(vk::DescriptorBufferInfo& vboBufferInfo,
                                           vk::DescriptorBufferInfo& iboBufferInfo) const;

    private:
        void AddMeshNodeGeometry(const vkfw_core::gfx::MeshInfo& mesh, const vkfw_core::gfx::SceneMeshNode* node,
                                 const glm::mat4& transform, std::size_t vertexSize,
                                 vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);

        void AddSubMeshGeometry(const vkfw_core::gfx::SubMesh& subMesh, const glm::mat4& transform,
                                std::size_t vertexSize, vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The bottom level acceleration structure for the scene. */
        std::vector<BottomLevelAccelerationStructure> m_BLAS;
        /** The transformations for the bottom level acceleration structures. */
        std::vector<glm::mat3x4> m_BLASTransforms;
        /** The top level acceleration structure for the scene. */
        TopLevelAccelerationStructure m_TLAS;

        struct MeshGeometryInfo
        {
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

        /** Holds the memory for geometry if needed. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
        /** Holds the information for the the meshes vertex and index buffers. */
        std::vector<MeshGeometryInfo> m_meshGeometryInfos;
    };

    template<Vertex VertexType> void vkfw_core::gfx::rt::AccelerationStructureGeometry::FinalizeMeshGeometry()
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
            meshInfo.iboOffset = meshInfo.vboRange;
            meshInfo.vertexSize = sizeof(VertexType);

            const std::size_t bufferSize = meshInfo.iboOffset + meshInfo.iboRange;

            meshInfo.bufferIndex = m_memGroup.AddBufferToGroup(
                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                    | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                bufferSize, std::vector<std::uint32_t>{{0, 1}});
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.vboOffset, vertices[i_mesh]);
            m_memGroup.AddDataToBufferInGroup(meshInfo.bufferIndex, meshInfo.iboOffset, indices[i_mesh]);

        }

        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, std::make_pair(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        for (std::size_t i_mesh = 0; i_mesh < m_meshGeometryInfos.size(); ++i_mesh) {
            const auto& meshInfo = m_meshGeometryInfos[i_mesh];

            vk::DeviceOrHostAddressConstKHR bufferDeviceAddress =
                m_memGroup.GetBuffer(meshInfo.bufferIndex)->GetDeviceAddressConst();
            vk::DeviceOrHostAddressConstKHR vboDeviceAddress = bufferDeviceAddress.deviceAddress + meshInfo.vboOffset;
            vk::DeviceOrHostAddressConstKHR iboDeviceAddress = bufferDeviceAddress.deviceAddress + meshInfo.iboOffset;

            AddMeshGeometry(*meshInfo.mesh, meshInfo.transform, sizeof(VertexType), vboDeviceAddress, iboDeviceAddress);
        }
    }

    template<Vertex VertexType>
    std::size_t AccelerationStructureGeometry::AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh,
                                                               const glm::mat4& transform)
    {
        auto meshIndex = m_meshGeometryInfos.size();

        auto& meshInfo = m_meshGeometryInfos.emplace_back(&mesh, transform);
        meshInfo.indices = mesh.GetIndices();

        std::vector<VertexType> vertices;
        mesh.GetVertices(vertices);

        meshInfo.vertexSize = sizeof(VertexType);
        meshInfo.vboRange = byteSizeOf(vertices);
        meshInfo.vboOffset = 0;
        meshInfo.iboRange = byteSizeOf(meshInfo.indices);
        meshInfo.iboOffset = meshInfo.vboRange;
        meshInfo.vertices.resize(byteSizeOf(vertices));
        memcpy(meshInfo.vertices.data(), vertices.data(), byteSizeOf(vertices));

        return meshIndex;
    }
}
