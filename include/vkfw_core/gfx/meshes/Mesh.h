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
#include "gfx/vk/UniformBufferObject.h"
#include "core/concepts.h"
#include <tuple>

namespace vkfw_core::gfx {

    class MeshInfo;
    class MemoryGroup;
    class DeviceBuffer;
    class QueuedDeviceTransfer;
    class RenderElement;
    class RenderList;
    class CameraBase;

    struct WorldMatrixUBO
    {
        glm::mat4 m_model;
        glm::mat4 m_normalMatrix;
    };

    class Mesh
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh();

        template<Vertex VertexType, class MaterialType>
        static Mesh CreateWithInternalMemoryGroup(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<Vertex VertexType, class MaterialType>
        static Mesh CreateWithMemoryGroup(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<Vertex VertexType, class MaterialType>
        static Mesh CreateInExternalBuffer(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

        template<Vertex VertexType, class MaterialType>
        static std::size_t CalculateBufferSize(const LogicalDevice* device, const MeshInfo* meshInfo,
            std::size_t offset, std::size_t numBackbuffers);

        void UploadMeshData(QueuedDeviceTransfer& transfer);
        void CreateDescriptorSets(std::size_t numBackbuffers);
        [[nodiscard]] vk::DescriptorSetLayout GetMaterialTexturesDescriptorLayout() const
        {
            return *m_materialTexturesDescriptorSetLayout;
        }
        [[nodiscard]] vk::DescriptorSetLayout GetMaterialBufferDescriptorLayout() const
        {
            return m_materialsUBO.GetDescriptorLayout();
        }
        [[nodiscard]] vk::DescriptorSetLayout GetWorldMatricesDescriptorLayout() const
        {
            return m_worldMatricesUBO.GetDescriptorLayout();
        }

        void TransferWorldMatrices(vk::CommandBuffer transferCmdBuffer, std::size_t backbufferIdx) const;

        void UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const;
        void UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const;

        void Draw(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout) const;
        void DrawNode(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout, const SceneMeshNode* node) const;
        void DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh& subMesh) const;

        void GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
            RenderList& renderList) const;
        void GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
            const SceneMeshNode* node, RenderList& renderList) const;
        void GetDrawElementsSubMesh(const glm::mat4& worldMatrix, const CameraBase& camera,
            const SubMesh& subMesh, RenderList& renderList) const;

    private:
        Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
            std::size_t numBackbuffers, const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices);

        Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
            std::size_t numBackbuffers, MemoryGroup& memoryGroup, unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices);

        template<Vertex VertexType, class MaterialType>
        void CreateBuffersInMemoryGroup(std::size_t offset, std::size_t numBackbuffers, const std::vector<std::uint32_t>& queueFamilyIndices);
        void CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices);

        void SetVertexBuffer(const DeviceBuffer* vtxBuffer, std::size_t offset);
        void SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the mesh info object containing vertex/index data. */
        std::shared_ptr<const MeshInfo> m_meshInfo;
        /** Holds the internal memory group. */
        std::unique_ptr<MemoryGroup> m_internalMemoryGroup;
        /** Holds the memory group used in this mesh. */
        MemoryGroup* m_memoryGroup;
        /** The index into the memory group. */
        unsigned int m_bufferIdx;
        /** Holds a pointer to the vertex buffer and an offset to the vertex data. */
        std::pair<const DeviceBuffer*, vk::DeviceSize> m_vertexBuffer;
        /** Holds a pointer to the index buffer and an offset to the index data. */
        std::pair<const DeviceBuffer*, vk::DeviceSize> m_indexBuffer;

        /** Holds the uniform buffer for the world matrices. */
        UniformBufferObject m_worldMatricesUBO;
        /** Holds the uniform buffer for the materials. */
        UniformBufferObject m_materialsUBO;

        /** Holds the meshes materials. */
        std::vector<Material> m_materials;
        /** The sampler for the materials textures. */
        vk::UniqueSampler m_textureSampler;
        /** The descriptor pool for mesh rendering. */
        vk::UniqueDescriptorPool m_descriptorPool;
        /** The descriptor set layout for materials in mesh rendering. */
        vk::UniqueDescriptorSetLayout m_materialTexturesDescriptorSetLayout;
        /** Holds the material descriptor sets. */
        std::vector<vk::DescriptorSet> m_materialTextureDescriptorSets;

        /** Holds the vertex and material data while the mesh is constructed. */
        std::vector<uint8_t> m_vertexMaterialData;
    };

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithInternalMemoryGroup(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
            numBackbuffers, memoryFlags, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithMemoryGroup(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
            numBackbuffers, memoryGroup, m_bufferIdx, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateInExternalBuffer(const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
            numBackbuffers, memoryGroup, m_bufferIdx, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(offset, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<Vertex VertexType, class MaterialType>
    inline void Mesh::CreateBuffersInMemoryGroup(std::size_t offset, std::size_t,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        // TODO: possible bug? numBackbuffers is not used currently. [3/28/2020 Sebastian Maisch]
        std::vector<VertexType> vertices;
        m_meshInfo->GetVertices(vertices);

        auto materialAlignment = m_device->CalculateUniformBufferAlignment(sizeof(MaterialType));
        aligned_vector<MaterialType> materialUBOContent{ materialAlignment }; materialUBOContent.reserve(m_materials.size());
        for (const auto& material : m_materials) materialUBOContent.emplace_back(material);

        WorldMatrixUBO worldMatrices;
        worldMatrices.m_model = glm::mat4{ 1.0f };
        worldMatrices.m_normalMatrix = glm::mat4{ 1.0f };

        auto vertexBufferSize = vkfw_core::byteSizeOf(vertices);
        auto indexBufferSize = vkfw_core::byteSizeOf(m_meshInfo->GetIndices());
        auto materialBufferSize = m_device->CalculateUniformBufferAlignment(byteSizeOf(materialUBOContent));

        m_vertexMaterialData.resize(vertexBufferSize + materialBufferSize + sizeof(WorldMatrixUBO));
        memcpy(m_vertexMaterialData.data(), vertices.data(), vertexBufferSize);
        memcpy(m_vertexMaterialData.data() + vertexBufferSize, materialUBOContent.data(), materialBufferSize);
        memcpy(m_vertexMaterialData.data() + vertexBufferSize + materialBufferSize, &worldMatrices, sizeof(WorldMatrixUBO));

        auto materialBufferAlignment = m_device->CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize);
        auto worldMatricesBufferAlignment = m_device->CalculateUniformBufferAlignment(materialBufferAlignment + materialBufferSize);

        if (m_bufferIdx == DeviceMemoryGroup::INVALID_INDEX) m_bufferIdx = m_memoryGroup->AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
            worldMatricesBufferAlignment + m_worldMatricesUBO.GetCompleteSize(), queueFamilyIndices);

        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, offset, vertexBufferSize, m_vertexMaterialData.data());
        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, offset + vertexBufferSize, m_meshInfo->GetIndices());

        m_materialsUBO.AddUBOToBufferPrefill(m_memoryGroup, m_bufferIdx, materialBufferAlignment,
            materialBufferSize, m_vertexMaterialData.data() + vertexBufferSize);
        m_worldMatricesUBO.AddUBOToBuffer(m_memoryGroup, m_bufferIdx, worldMatricesBufferAlignment,
            *reinterpret_cast<const WorldMatrixUBO*>(m_vertexMaterialData.data() + vertexBufferSize + materialBufferSize));

        auto buffer = m_memoryGroup->GetBuffer(m_bufferIdx);
        SetVertexBuffer(buffer, offset);
        SetIndexBuffer(buffer, offset + vertexBufferSize);
    }

    template<Vertex VertexType, class MaterialType>
    inline std::size_t Mesh::CalculateBufferSize(const LogicalDevice* device, const MeshInfo* meshInfo, std::size_t offset, std::size_t numBackbuffers)
    {
        auto materialAlignment = device->CalculateUniformBufferAlignment(sizeof(MaterialType));
        auto localMatricesAlignment = device->CalculateUniformBufferAlignment(2 * sizeof(glm::mat4));

        auto vertexBufferSize = meshInfo->GetVertices().size() * sizeof(VertexType);
        auto indexBufferSize = meshInfo->GetIndices().size() * sizeof(std::uint32_t);
        auto materialBufferSize = device->CalculateUniformBufferAlignment(meshInfo->GetMaterials().size() * materialAlignment);
        auto localMatricesBufferSize = device->CalculateUniformBufferAlignment(meshInfo->GetNodes().size() * localMatricesAlignment);

        auto materialBufferAlignment = device->CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize);
        auto localMatricesBufferAlignment = device->CalculateUniformBufferAlignment(materialBufferAlignment + materialBufferSize);

        return localMatricesBufferAlignment + numBackbuffers * localMatricesBufferSize;
    }
}
