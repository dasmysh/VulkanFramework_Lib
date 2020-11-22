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

        template<Vertex VertexType>
        void AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh, const glm::mat4& transform);
        void AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh, const glm::mat4& transform, std::size_t vertexSize,
                             vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                             vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);

        std::size_t AddBottomLevelAccelerationStructure(const glm::mat3x4& transform);
        BottomLevelAccelerationStructure& GetBottomLevelAccelerationStructure(std::size_t index) { return m_BLAS[index]; }
        glm::mat3x4& GetBottomLevelAccelerationStructureTransform(std::size_t index) { return m_BLASTransforms[index]; }

        void BuildAccelerationStructure();

        void FillDescriptorLayoutBinding(vk::DescriptorSetLayoutBinding& asLayoutBinding,
                                         const vk::ShaderStageFlags& shaderFlags, std::uint32_t binding = 0) const;
        void CreateLayout(vk::DescriptorPool descPool, const vk::ShaderStageFlags& shaderFlags,
                          std::uint32_t binding = 0);
        void UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, std::uint32_t binding = 0);
        void UseDescriptorSet(vk::DescriptorSet descSet, vk::DescriptorSetLayout usedLayout, std::uint32_t binding = 0);

        void FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const;

    private:
        void AddMeshNodeGeometry(const vkfw_core::gfx::MeshInfo& mesh, const vkfw_core::gfx::SceneMeshNode* node,
                                 const glm::mat4& transform, std::size_t vertexSize,
                                 vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);

        void AddSubMeshGeometry(const vkfw_core::gfx::SubMesh& subMesh, const glm::mat4& transform,
                                std::size_t vertexSize, vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress);
        void AllocateDescriptorSet(vk::DescriptorPool descPool);

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The bottom level acceleration structure for the scene. */
        std::vector<BottomLevelAccelerationStructure> m_BLAS;
        /** The transformations for the bottom level acceleration structures. */
        std::vector<glm::mat3x4> m_BLASTransforms;
        /** The top level acceleration structure for the scene. */
        TopLevelAccelerationStructure m_TLAS;

        /** Contains the descriptor binding. */
        std::uint32_t m_descBinding = 0;
        /** The internal descriptor layout if created here. */
        vk::UniqueDescriptorSetLayout m_internalDescLayout;
        /** The descriptor layout used. */
        vk::DescriptorSetLayout m_descLayout = vk::DescriptorSetLayout();
        /** The descriptor set of this buffer. */
        vk::DescriptorSet m_descSet = vk::DescriptorSet();

        /** The descriptor set acceleration structure write info (needed to ensure a valid pointer). */
        vk::WriteDescriptorSetAccelerationStructureKHR m_descriptorSetAccStructure;

        /** Holds the memory for geometry if needed. */
        vkfw_core::gfx::MemoryGroup m_memGroup;
    };


    template<Vertex VertexType>
    void AccelerationStructureGeometry::AddMeshGeometry(const vkfw_core::gfx::MeshInfo& mesh,
                                                        const glm::mat4& transform)
    {
        std::vector<VertexType> verticesMesh;
        mesh.GetVertices(verticesMesh);
        auto& indicesMesh = mesh.GetIndices();

        auto indexBufferMeshOffset = vkfw_core::byteSizeOf(verticesMesh);
        auto completeBufferSize = indexBufferMeshOffset + sizeof(std::uint32_t) * indicesMesh.size();
        auto completeBufferIdx = m_memGroup.AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            completeBufferSize, std::vector<std::uint32_t>{{0, 1}});

        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, 0, verticesMesh);
        m_memGroup.AddDataToBufferInGroup(completeBufferIdx, indexBufferMeshOffset, indicesMesh);

        vkfw_core::gfx::QueuedDeviceTransfer transfer{m_device, std::make_pair(0, 0)};
        m_memGroup.FinalizeDeviceGroup();
        m_memGroup.TransferData(transfer);
        transfer.FinishTransfer();

        vk::DeviceOrHostAddressConstKHR vertexBufferMeshDeviceAddress =
            m_memGroup.GetBuffer(completeBufferIdx)->GetDeviceAddressConst();
        vk::DeviceOrHostAddressConstKHR indexBufferMeshDeviceAddress{vertexBufferMeshDeviceAddress.deviceAddress
                                                                     + indexBufferMeshOffset};

        AddMeshGeometry(mesh, transform, sizeof(VertexType), vertexBufferMeshDeviceAddress,
                        indexBufferMeshDeviceAddress);
    }
}
