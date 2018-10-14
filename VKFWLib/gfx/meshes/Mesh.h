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
#include <tuple>

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

        template<class VertexTypev, class MaterialType>
        static Mesh CreateWithInternalMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<class VertexType, class MaterialType>
        static Mesh CreateWithMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        template<class VertexType, class MaterialType>
        static Mesh CreateInExternalBuffer(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
            const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

        template<class VertexType, class MaterialType>
        static std::size_t CalculateBufferSize(const LogicalDevice* device, const MeshInfo* meshInfo,
            std::size_t offset, std::size_t numBackbuffers);

        void UploadMeshData(QueuedDeviceTransfer& transfer);
        void CreateDescriptorSets(std::size_t numBackbuffers);
        vk::DescriptorSetLayout GetMaterialDescriptorLayout() const { return *materialDescriptorSetLayout_; }
        vk::DescriptorSetLayout GetWorldMatricesDescriptorLayout() const { return *worldMatricesDescriptorSetLayout_; }

        void TransferWorldMatrices(vk::CommandBuffer transferCmdBuffer, std::size_t backbufferIdx) const;

        void UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const;
        void UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const;

        void Draw(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout) const;
        void DrawNode(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SceneMeshNode* node) const;
        void DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh* subMesh) const;

    private:
        Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
            vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices);
        Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
            MemoryGroup& memoryGroup, unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices);

        template<class VertexType, class MaterialType>
        void CreateBuffersInMemoryGroup(std::size_t offset, std::size_t numBackbuffers, const std::vector<std::uint32_t>& queueFamilyIndices);
        void CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices);

        void SetVertexBuffer(const DeviceBuffer* vtxBuffer, std::size_t offset);
        void SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset);
        void SetMaterialBuffer(const DeviceBuffer* matBuffer, std::size_t offset, std::size_t elementSize);
        void SetWorldMatricesBuffer(const DeviceBuffer* worldMatricesBuffer, std::size_t offset, std::size_t bufferSize, std::size_t elementSize);

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the mesh info object containing vertex/index data. */
        std::shared_ptr<const MeshInfo> meshInfo_;
        /** Holds the internal memory group. */
        std::unique_ptr<MemoryGroup> internalMemoryGroup_;
        /** Holds the memory group used in this mesh. */
        MemoryGroup* memoryGroup_;
        /** The index into the memory group. */
        unsigned int bufferIdx_;
        /** Holds a pointer to the vertex buffer and an offset to the vertex data. */
        std::pair<const DeviceBuffer*, std::size_t> vertexBuffer_;
        /** Holds a pointer to the index buffer and an offset to the index data. */
        std::pair<const DeviceBuffer*, std::size_t> indexBuffer_;
        /** Holds a pointer to the material buffer and an offset to the material data. */
        std::tuple<const DeviceBuffer*, std::size_t, std::size_t> materialBuffer_;
        /** Holds a pointer to the local matrices buffer and an offset to the local matrices data. */
        std::tuple<const DeviceBuffer*, std::size_t, std::size_t, std::size_t> worldMatricesBuffer_;
        /** Holds the meshes materials. */
        std::vector<Material> materials_;
        /** The sampler for the materials textures. */
        vk::UniqueSampler textureSampler_;
        /** The descriptor pool for mesh rendering. */
        vk::UniqueDescriptorPool descriptorPool_;
        /** The descriptor set layout for materials in mesh rendering. */
        vk::UniqueDescriptorSetLayout materialDescriptorSetLayout_;
        /** Holds the material descriptor sets. */
        std::vector<vk::DescriptorSet> materialDescriptorSets_;
        /** The descriptor set layout for local matrices in mesh rendering. */
        vk::UniqueDescriptorSetLayout worldMatricesDescriptorSetLayout_;
        /** Holds the local matrices descriptor sets. */
        std::vector<vk::DescriptorSet> worldMatricesDescriptorSets_;

        /** Holds the size of a single material in the buffer. */

        /** Holds the vertex and material data while the mesh is constructed. */
        std::vector<uint8_t> vertexMaterialData_;
    };

    template<class VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithInternalMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, memoryFlags, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<class VertexType, class MaterialType>
    inline Mesh Mesh::CreateWithMemoryGroup(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, memoryGroup, bufferIdx_, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(0, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<class VertexType, class MaterialType>
    inline Mesh Mesh::CreateInExternalBuffer(std::shared_ptr<const MeshInfo> meshInfo, std::size_t numBackbuffers,
        const LogicalDevice* device, MemoryGroup& memoryGroup, unsigned int bufferIdx, std::size_t offset,
        const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        Mesh result{ meshInfo, device, memoryGroup, bufferIdx_, queueFamilyIndices };
        result.CreateBuffersInMemoryGroup<VertexType, MaterialType>(offset, numBackbuffers, queueFamilyIndices);
        return result;
    }

    template<class VertexType, class MaterialType>
    inline void Mesh::CreateBuffersInMemoryGroup(std::size_t offset, std::size_t numBackbuffers, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        std::vector<VertexType> vertices;
        meshInfo_->GetVertices(vertices);

        auto materialAlignment = device_->CalculateUniformBufferAlignment(sizeof(MaterialType));
        aligned_vector<MaterialType> materialUBOContent{ materialAlignment }; materialUBOContent.reserve(materials_.size());
        for (const auto& material : materials_) materialUBOContent.emplace_back(material);

        auto worldMatricesAlignment = device_->CalculateUniformBufferAlignment(2 * sizeof(glm::mat4));
        std::vector<aligned_vector<std::array<glm::mat4, 2>>> worldMatricesUBOContent;
        for (std::size_t i = 0; i < numBackbuffers; ++i) {
            worldMatricesUBOContent.emplace_back(worldMatricesAlignment);
            worldMatricesUBOContent[i].resize(meshInfo_->GetNodes().size());
            for (const auto& node : meshInfo_->GetNodes()) {
                worldMatricesUBOContent[i][node->GetNodeIndex()][0] = glm::mat4{ 1.0f };
                worldMatricesUBOContent[i][node->GetNodeIndex()][1] = glm::mat4{ 1.0f };
            }
        }
        // localMatricesUBOContent.resize(numBackbuffers * meshInfo_->GetNodes().size());
        // for (std::size_t i = 0; i < numBackbuffers; ++i)
        //     for (const auto& node : meshInfo_->GetNodes())
        //         localMatricesUBOContent.emplace_back({ glm::mat4{ 1.0f }, glm::mat4{ 1.0f } });


        auto vertexBufferSize = vku::byteSizeOf(vertices);
        auto indexBufferSize = vku::byteSizeOf(meshInfo_->GetIndices());
        auto materialBufferSize = device_->CalculateUniformBufferAlignment(byteSizeOf(materialUBOContent));
        auto worldMatricesBufferSize = device_->CalculateUniformBufferAlignment(worldMatricesAlignment * meshInfo_->GetNodes().size());

        vertexMaterialData_.resize(vertexBufferSize + materialBufferSize + numBackbuffers * worldMatricesBufferSize);
        memcpy(vertexMaterialData_.data(), vertices.data(), vertexBufferSize);
        memcpy(vertexMaterialData_.data() + vertexBufferSize, materialUBOContent.data(), materialBufferSize);
        for (std::size_t i = 0; i < numBackbuffers; ++i)
            memcpy(vertexMaterialData_.data() + vertexBufferSize + materialBufferSize + i * worldMatricesBufferSize,
                worldMatricesUBOContent[i].data(), worldMatricesBufferSize);

        auto materialBufferAlignment = device_->CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize);
        auto worldMatricesBufferAlignment = device_->CalculateUniformBufferAlignment(materialBufferAlignment + materialBufferSize);

        if (bufferIdx_ == DeviceMemoryGroup::INVALID_INDEX) bufferIdx_ = memoryGroup_->AddBufferToGroup(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer,
            worldMatricesBufferAlignment + numBackbuffers * worldMatricesBufferSize, queueFamilyIndices);

        memoryGroup_->AddDataToBufferInGroup(bufferIdx_, offset, vertexBufferSize, vertexMaterialData_.data());
        memoryGroup_->AddDataToBufferInGroup(bufferIdx_, offset + vertexBufferSize, meshInfo_->GetIndices());
        memoryGroup_->AddDataToBufferInGroup(bufferIdx_, materialBufferAlignment, materialBufferSize, vertexMaterialData_.data() + vertexBufferSize);
        memoryGroup_->AddDataToBufferInGroup(bufferIdx_, worldMatricesBufferAlignment, numBackbuffers * worldMatricesBufferSize, vertexMaterialData_.data() + vertexBufferSize + materialBufferSize);

        // for (std::size_t i = 0; i < numBackbuffers; ++i) {
        //     memoryGroup_->AddDataToBufferInGroup(bufferIdx_, worldMatricesBufferAlignment + (i * worldMatricesBufferSize),
        //         worldMatricesUBOContent[i]);
        // }

        // memoryGroup->AddDataToBufferInGroup(bufferIdx, localMatricesBufferAlignment + (i * singleUBOSize), localMatricesBufferSize, vertexMaterialData_.data() + vertexBufferSize + materialBufferSize);
        // 
        // 
        // for (auto i = 0; i < numUBOBuffers; ++i) memGroup_.AddDataToBufferInGroup(completeBufferIdx_,
        //     uniformDataOffset_ + (i * singleUBOSize), sizeof(MVPMatrixUBO), &initialUBO);

        auto buffer = memoryGroup_->GetBuffer(bufferIdx_);
        SetVertexBuffer(buffer, offset);
        SetIndexBuffer(buffer, offset + vertexBufferSize);
        SetMaterialBuffer(buffer, materialBufferAlignment, materialAlignment);
        SetWorldMatricesBuffer(buffer, worldMatricesBufferAlignment, worldMatricesBufferSize, worldMatricesAlignment);
    }

    template<class VertexType, class MaterialType>
    inline std::size_t Mesh::CalculateBufferSize(const LogicalDevice* device, const MeshInfo* meshInfo, std::size_t offset, std::size_t numBackbuffers)
    {
        auto materialAlignment = device->CalculateUniformBufferAlignment(sizeof(MaterialType));
        auto localMatricesAlignment = device->CalculateUniformBufferAlignment(2 * sizeof(glm::mat4));

        auto vertexBufferSize = meshInfo->GetVertices().size() * sizeof(VertexType);
        auto indexBufferSize = meshInfo->GetIndices().size() * sizeof(std::uint32_t);
        auto materialBufferSize = device->CalculateUniformBufferAlignment(meshInfo->GetMaterials().size() * materialAlignment);
        auto localMatricesBufferSize = device_->CalculateUniformBufferAlignment(meshInfo->GetNodes().size() * localMatricesAlignment);

        auto materialBufferAlignment = device->CalculateUniformBufferAlignment(offset + vertexBufferSize + indexBufferSize);
        auto localMatricesBufferAlignment = device->CalculateUniformBufferAlignment(materialBufferAlignment + materialBufferSize);

        return localMatricesBufferAlignment + numBackbuffers * localMatricesBufferSize;
    }
}
