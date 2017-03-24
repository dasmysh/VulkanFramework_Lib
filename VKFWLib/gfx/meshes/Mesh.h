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

    class Mesh
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh();

        template<class VertexType>
        static Mesh CreateWithInternalMemoryGroup(const std::shared_ptr<MeshInfo>& meshInfo,
            const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{}) const;
        template<class VertexType>
        static Mesh CreateWithMemoryGroup(const std::shared_ptr<MeshInfo>& meshInfo,
            const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{}) const;
        template<class VertexType>
        static Mesh CreateInExternalBuffer(const std::shared_ptr<MeshInfo>& meshInfo,
            const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{}) const;

        template<class VertexType>
        static std::size_t CalculateBufferSize(const MeshInfo* meshInfo) const;

        void BindBuffersToCommandBuffer(vk::CommandBuffer cmdBuffer) const;

    private:
        Mesh(const std::shared_ptr<MeshInfo>& meshInfo, const LogicalDevice* device,
            vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices);
        Mesh(const std::shared_ptr<MeshInfo>& meshInfo, const LogicalDevice* device,
            MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices);

        template<class VertexType>
        void CreateBuffersInMemoryGroup(MemoryGroup* memoryGroup, unsigned int bufferIdx, std::size_t offset,
            const std::vector<std::uint32_t>& queueFamilyIndices);
        void CreateMaterials(const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices);

        void SetVertexBuffer(const DeviceBuffer* vtxBuffer, std::size_t offset);
        void SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset);

        /** Holds the mesh info object containing vertex/index data. */
        std::shared_ptr<MeshInfo> meshInfo_;
        /** Holds the internal memory group. */
        std::unique_ptr<MemoryGroup> memoryGroup_;
        /** Holds a pointer to the vertex buffer and an offset to the vertex data. */
        std::pair<const DeviceBuffer*, std::size_t> vertexBuffer_;
        /** Holds a pointer to the vertex buffer and an offset to the index data. */
        std::pair<const DeviceBuffer*, std::size_t> indexBuffer_;
        /** Holds the meshes materials. */
        std::vector<Material> materials_;

    };
    template<class VertexType>
    inline Mesh Mesh::CreateWithInternalMemoryGroup(const std::shared_ptr<MeshInfo>& meshInfo, const LogicalDevice* device,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) const
    {
        Mesh result{ meshInfo, device, memoryFlags, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup(result.memoryGroup_.get(), DeviceMemoryGroup::INVALID_INDEX, 0, queueFamilyIndices);
        return result;
    }

    template<class VertexType>
    inline Mesh Mesh::CreateWithMemoryGroup(const std::shared_ptr<MeshInfo>& meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) const
    {
        Mesh result{ meshInfo, device, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup(memoryGroup, DeviceMemoryGroup::INVALID_INDEX, 0, queueFamilyIndices);
        return result;
    }

    template<class VertexType>
    inline Mesh Mesh::CreateInExternalBuffer(const std::shared_ptr<MeshInfo>& meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices) const
    {
        Mesh result{ meshInfo, device, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup(memoryGroup, bufferIdx, offset, queueFamilyIndices);
        return result;
    }

    template<class VertexType>
    inline void Mesh::CreateBuffersInMemoryGroup(MemoryGroup* memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        std::vector<VertexType> vertices;
        meshInfo_->GetVertices(vertices);
        auto vertexBufferSize = vku::byteSizeOf(vertices);
        auto indexBufferSize = vku::byteSizeOf(meshInfo_->GetIndices());

        if (bufferIdx == DeviceMemoryGroup::INVALID_INDEX) bufferIdx = memoryGroup->AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
            vertexBufferSize + indexBufferSize, queueFamilyIndices);

        memoryGroup->AddDataToBufferInGroup(bufferIdx, offset, vertices);
        memoryGroup->AddDataToBufferInGroup(bufferIdx, offset + vertexBufferSize, meshInfo_->GetIndices());
        // TODO: any kind of uniform data to add here? [3/23/2017 Sebastian Maisch]

        auto buffer = memoryGroup_->GetBuffer(bufferIdx);
        SetVertexBuffer(buffer, offset);
        SetIndexBuffer(buffer, offset + vertexBufferSize);
    }

    template<class VertexType>
    inline std::size_t Mesh::CalculateBufferSize(const MeshInfo* meshInfo) const
    {
        return static_cast<std::size_t>(sizeof(VertexType) * meshInfo->GetVertices().size()
            + sizeof(std::uint32_t) * meshInfo->GetIndices().size());
    }
}
