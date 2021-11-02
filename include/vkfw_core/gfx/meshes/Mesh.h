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
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/Material.h"
#include "MeshInfo.h"
#include "gfx/vk/UniformBufferObject.h"
#include "gfx/vk/wrappers/DescriptorSet.h"
#include "gfx/vk/wrappers/PipelineLayout.h"
#include "core/concepts.h"
#include "mesh/mesh_host_interface.h"
#include <tuple>

namespace vkfw_core::gfx {

    class MeshInfo;
    class MemoryGroup;
    class DeviceBuffer;
    class QueuedDeviceTransfer;
    class RenderElement;
    class RenderList;
    class CameraBase;
    class VertexInputResources;

    class Mesh
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh();

        template<Vertex VertexType, class MaterialType>
        static Mesh CreateWithInternalMemoryGroup(
            std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<Vertex VertexType, class MaterialType>
        static Mesh
        CreateWithMemoryGroup(std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
                              std::size_t numBackbuffers, const LogicalDevice* device, MemoryGroup& memoryGroup,
                              const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<Vertex VertexType, class MaterialType>
        static Mesh
        CreateInExternalBuffer(std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
                               std::size_t numBackbuffers, const LogicalDevice* device, MemoryGroup& memoryGroup,
                               unsigned int bufferIdx, std::size_t offset,
                               const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

        template<Vertex VertexType, class MaterialType>
        static std::size_t CalculateBufferSize(const LogicalDevice* device, const MeshInfo* meshInfo,
            std::size_t offset, std::size_t numBackbuffers);

        void UploadMeshData(QueuedDeviceTransfer& transfer);
        void AddDescriptorPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, std::size_t& setCount) const;
        void CreateDescriptorSets(const DescriptorPool& descriptorPool);
        [[nodiscard]] const DescriptorSetLayout& GetMaterialDescriptorLayout() const
        {
            return m_materialDescriptorSetLayout;
        }
        [[nodiscard]] const DescriptorSetLayout& GetWorldMatricesDescriptorLayout() const
        {
            return m_worldMatricesDescriptorSetLayout;
        }

        void TransferWorldMatrices(CommandBuffer& transferCmdBuffer, std::size_t backbufferIdx) const;

        void UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const;
        void UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const;

        void Draw(CommandBuffer& cmdBuffer, std::size_t backbufferIdx,
                  const PipelineLayout& pipelineLayout);
        void DrawNode(CommandBuffer& cmdBuffer, std::size_t backbufferIdx, const PipelineLayout& pipelineLayout,
                      const SceneMeshNode* node);
        void DrawSubMesh(CommandBuffer& cmdBuffer, const PipelineLayout& pipelineLayout,
                         const SubMesh& subMesh);

        void GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
            RenderList& renderList);
        void GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
            const SceneMeshNode* node, RenderList& renderList);
        void GetDrawElementsSubMesh(const glm::mat4& worldMatrix, const CameraBase& camera,
            const SubMesh& subMesh, RenderList& renderList);

    private:
        Mesh(const LogicalDevice* device, std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
             UniformBufferObject&& materialsUBO, std::size_t numBackbuffers, const vk::MemoryPropertyFlags& memoryFlags,
             const std::vector<std::uint32_t>& queueFamilyIndices);
        Mesh(const LogicalDevice* device, std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
             UniformBufferObject&& materialsUBO, std::size_t numBackbuffers, MemoryGroup& memoryGroup,
             unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices);

        template<Vertex VertexType, class MaterialType>
        void CreateBuffersInMemoryGroup(std::size_t offset, std::size_t numBackbuffers, const std::vector<std::uint32_t>& queueFamilyIndices);
        void CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices);

        void SetVertexInput(DeviceBuffer* vtxBuffer, std::size_t vtxOffset, DeviceBuffer* idxBuffer, std::size_t idxOffset);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the name. */
        std::string m_name;
        /** Holds the mesh info object containing vertex/index data. */
        std::shared_ptr<const MeshInfo> m_meshInfo;
        /** Holds the internal memory group. */
        std::unique_ptr<MemoryGroup> m_internalMemoryGroup;
        /** Holds the memory group used in this mesh. */
        MemoryGroup* m_memoryGroup;
        /** The index into the memory group. */
        unsigned int m_bufferIdx;
        /** Holds the vertex input resources. */
        std::unique_ptr<VertexInputResources> m_vertexInput;

        /** Holds the uniform buffer for the world matrices. */
        UniformBufferObject m_worldMatricesUBO;
        /** Holds the uniform buffer for the materials. */
        UniformBufferObject m_materialsUBO;

        /** Holds the meshes materials. */
        std::vector<Material> m_materials;
        /** The sampler for the materials textures. */
        Sampler m_textureSampler;
        /** The descriptor set layout for world matrices in mesh rendering. */
        DescriptorSetLayout m_worldMatricesDescriptorSetLayout;
        /** Holds the world matrix descriptor set. */
        DescriptorSet m_worldMatrixDescriptorSet;
        /** The descriptor set layout for materials in mesh rendering. */
        DescriptorSetLayout m_materialDescriptorSetLayout;
        /** Holds the material descriptor sets. */
        std::vector<DescriptorSet> m_materialDescriptorSets;

        /** Holds the vertex and material data while the mesh is constructed. */
        std::vector<uint8_t> m_vertexMaterialData;
    };

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithInternalMemoryGroup(std::string_view name,
                                                    const std::shared_ptr<const MeshInfo>& meshInfo,
                                                    std::size_t numBackbuffers,
        const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, name, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
            numBackbuffers, memoryFlags, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithMemoryGroup(std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
                                            std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, name, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
            numBackbuffers, memoryGroup, m_bufferIdx, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<Vertex VertexType, class MaterialType>
    inline Mesh Mesh::CreateInExternalBuffer(std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
                                             std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ device, name, meshInfo, UniformBufferObject::Create<MaterialType>(device, meshInfo->GetMaterials().size()),
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

        mesh::WorldUniformBufferObject worldMatrices;
        worldMatrices.model = glm::mat4{ 1.0f };
        worldMatrices.normalMatrix = glm::mat4{ 1.0f };

        auto vertexBufferSize = vkfw_core::byteSizeOf(vertices);
        auto indexBufferSize = vkfw_core::byteSizeOf(m_meshInfo->GetIndices());
        auto materialBufferSize = m_device->CalculateUniformBufferAlignment(byteSizeOf(materialUBOContent));

        m_vertexMaterialData.resize(vertexBufferSize + materialBufferSize + sizeof(mesh::WorldUniformBufferObject));
        memcpy(m_vertexMaterialData.data(), vertices.data(), vertexBufferSize);
        memcpy(m_vertexMaterialData.data() + vertexBufferSize, materialUBOContent.data(), materialBufferSize);
        memcpy(m_vertexMaterialData.data() + vertexBufferSize + materialBufferSize, &worldMatrices,
               sizeof(mesh::WorldUniformBufferObject));

        auto materialBufferAlignment = m_device->CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize);
        auto worldMatricesBufferAlignment = m_device->CalculateUniformBufferAlignment(materialBufferAlignment + materialBufferSize);

        if (m_bufferIdx == DeviceMemoryGroup::INVALID_INDEX)
            m_bufferIdx = m_memoryGroup->AddBufferToGroup(
                fmt::format("{}:Buffer", m_name),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
            worldMatricesBufferAlignment + m_worldMatricesUBO.GetCompleteSize(), queueFamilyIndices);

        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, offset, vertexBufferSize, m_vertexMaterialData.data());
        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, offset + vertexBufferSize, m_meshInfo->GetIndices());

        m_materialsUBO.AddUBOToBufferPrefill(m_memoryGroup, m_bufferIdx, materialBufferAlignment,
            materialBufferSize, m_vertexMaterialData.data() + vertexBufferSize);
        m_worldMatricesUBO.AddUBOToBuffer(m_memoryGroup, m_bufferIdx, worldMatricesBufferAlignment,
                                          *reinterpret_cast<const mesh::WorldUniformBufferObject*>(
                                              m_vertexMaterialData.data() + vertexBufferSize + materialBufferSize));

        auto buffer = m_memoryGroup->GetBuffer(m_bufferIdx);
        SetVertexInput(buffer, offset, buffer, offset + vertexBufferSize);
        // SetVertexBuffer(buffer, offset);
        // SetIndexBuffer(buffer, offset + vertexBufferSize);
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
