/**
 * @file   Mesh.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.23
 *
 * @brief  Declaration of the Mesh class.
 */

#pragma once

#include "main.h"
#include "core/type_traits.h"
#include "gfx/Material.h"
#include "MeshInfo.h"

namespace vku::gfx {

    class MeshInfo;
    class MemoryGroup;
    class DeviceBuffer;
    class QueuedDeviceTransfer;

    class Mesh
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh();

        template<class VertexType>
        static Mesh CreateWithInternalMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo,
            const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<class VertexType>
        static Mesh CreateWithMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo,
            const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<class VertexType>
        static Mesh CreateInExternalBuffer(std::shared_ptr<const MeshInfo> meshInfo,
            const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

        template<class VertexType, class MaterialType>
        static std::size_t CalculateBufferSize(const MeshInfo* meshInfo);

        void UploadMeshData(QueuedDeviceTransfer& transfer);

        void BindBuffersToCommandBuffer(vk::CommandBuffer cmdBuffer) const;
        void DrawMesh(vk::CommandBuffer cmdBuffer, const glm::mat4& worldMatrix) const;
        void DrawMeshNode(vk::CommandBuffer cmdBuffer, const SceneMeshNode* node, const glm::mat4& worldMatrix) const;
        void DrawSubMesh(vk::CommandBuffer cmdBuffer, const SubMesh* subMesh, const glm::mat4& worldMatrix) const;

    private:
        Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
            vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices);
        Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
            MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices);

        template<class VertexType, class MaterialType>
        void CreateBuffersInMemoryGroup(const LogicalDevice* device, MemoryGroup* memoryGroup,
            unsigned int bufferIdx, std::size_t offset, const std::vector<std::uint32_t>& queueFamilyIndices);
        void CreateMaterials(const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices);

        void SetVertexBuffer(const DeviceBuffer* vtxBuffer, std::size_t offset);
        void SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset);
        void SetMaterialBuffer(const DeviceBuffer* matBuffer, std::size_t offset);


        struct MaterialDescriptorSet
        {
            vk::DescriptorSet materialUBODescriptorSet_;
            vk::DescriptorSet diffuseTexDescriptorSet_;
            vk::DescriptorSet bumpTexDescriptorSet_;
        };

        /** Holds the mesh info object containing vertex/index data. */
        std::shared_ptr<const MeshInfo> meshInfo_;
        /** Holds the internal memory group. */
        std::unique_ptr<MemoryGroup> memoryGroup_;
        /** Holds a pointer to the vertex buffer and an offset to the vertex data. */
        std::pair<const DeviceBuffer*, std::size_t> vertexBuffer_;
        /** Holds a pointer to the index buffer and an offset to the index data. */
        std::pair<const DeviceBuffer*, std::size_t> indexBuffer_;
        /** Holds a pointer to the material buffer and an offset to the material data. */
        std::pair<const DeviceBuffer*, std::size_t> materialBuffer_;
        /** Holds the meshes materials. */
        std::vector<Material> materials_;
        /** The descriptor set for the material UBO. */
        // 
        /** Holds the material descriptor sets. */
        std::vector<MaterialDescriptorSet> materialDescriptorSets_;

        /** Holds the size of a single material in the buffer. */

        /** Holds the vertex and material data while the mesh is constructed. */
        std::vector<uint8_t> vertexMaterialData_;
    };

    template<class VertexType>
    inline Mesh Mesh::CreateWithInternalMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, memoryFlags, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType>(result.memoryGroup_.get(), DeviceMemoryGroup::INVALID_INDEX, 0, queueFamilyIndices);
        return result;
    }

    template<class VertexType>
    inline Mesh Mesh::CreateWithMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType>(memoryGroup, DeviceMemoryGroup::INVALID_INDEX, 0, queueFamilyIndices);
        return result;
    }

    template<class VertexType>
    inline Mesh Mesh::CreateInExternalBuffer(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType>(memoryGroup, bufferIdx, offset, queueFamilyIndices);
        return result;
    }

    template<class VertexType, class MaterialType>
    inline void Mesh::CreateBuffersInMemoryGroup(const LogicalDevice* device, MemoryGroup* memoryGroup,
        unsigned int bufferIdx, std::size_t offset, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        std::vector<VertexType> vertices;
        meshInfo_->GetVertices(vertices);

        std::vector<MaterialType> materials; materials.reserve(materials_.size());
        for (const auto& material : materials_) materials.emplace_back(material);

        auto vertexBufferSize = vku::byteSizeOf(vertices);
        auto indexBufferSize = vku::byteSizeOf(meshInfo_->GetIndices());
        auto materialBufferSize = device.CalculateUniformBufferAlignment(byteSizeOf(materials));

        vertexMaterialData_.resize(vertexBufferSize + materialBufferSize);
        memcpy(vertexMaterialData_.data(), vertices.data(), vertexBufferSize);
        memcpy(vertexMaterialData_.data() + vertexBufferSize, materials.data(), materialBufferSize);

        if (bufferIdx == DeviceMemoryGroup::INVALID_INDEX) bufferIdx = memoryGroup->AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
            vertexBufferSize + indexBufferSize, queueFamilyIndices);

        memoryGroup->AddDataToBufferInGroup(bufferIdx, offset, vertexBufferSize, vertexMaterialData_.data());
        memoryGroup->AddDataToBufferInGroup(bufferIdx, offset + vertexBufferSize, meshInfo_->GetIndices());
        memoryGroup->AddDataToBufferInGroup(bufferIdx, offset + vertexBufferSize + indexBufferSize,
            materialBufferSize, vertexMaterialData_.data() + vertexBufferSize);

        auto buffer = memoryGroup_->GetBuffer(bufferIdx);
        SetVertexBuffer(buffer, offset);
        SetIndexBuffer(buffer, offset + vertexBufferSize);
        SetMaterialBuffer(buffer, device.CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize));
    }

    template<class VertexType, class MaterialType>
    inline std::size_t Mesh::CalculateBufferSize(const MeshInfo* meshInfo)
    {
        return static_cast<std::size_t>(sizeof(VertexType) * meshInfo->GetVertices().size()
            + sizeof(std::uint32_t) * meshInfo->GetIndices().size()
            + sizeof(MaterialType) * meshInfo->GetMaterials().size());
    }
}
