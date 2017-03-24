/**
 * @file   Mesh.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.24
 *
 * @brief  Implementation of the Mesh class.
 */

#include "Mesh.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/Texture2D.h"

namespace vku::gfx {
    Mesh::Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        meshInfo_{ meshInfo },
        memoryGroup_{ std::make_unique<MemoryGroup>(device, memoryFlags) },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 }
    {
        CreateMaterials(device, *memoryGroup_.get(), queueFamilyIndices);
    }

    Mesh::Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        meshInfo_{ meshInfo },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 }
    {
        CreateMaterials(device, memoryGroup, queueFamilyIndices);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept :
        meshInfo_{ std::move(rhs.meshInfo_) },
        memoryGroup_{ std::move(rhs.memoryGroup_) },
        vertexBuffer_{ std::move(rhs.vertexBuffer_) },
        indexBuffer_{ std::move(rhs.indexBuffer_) },
        materials_{ std::move(rhs.materials_) },
        vertexData_{ std::move(rhs.vertexData_) }
    {
    }

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept
    {
        this->~Mesh();
        meshInfo_ = std::move(rhs.meshInfo_);
        memoryGroup_ = std::move(rhs.memoryGroup_);
        vertexBuffer_ = std::move(rhs.vertexBuffer_);
        indexBuffer_ = std::move(rhs.indexBuffer_);
        materials_ = std::move(rhs.materials_);
        vertexData_ = std::move(rhs.vertexData_);
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        materials_.reserve(meshInfo_->GetMaterials().size());
        for (const auto& mat : meshInfo_->GetMaterials()) {
            materials_.push_back(Material(&mat, device, memoryGroup, queueFamilyIndices));
        }
    }

    void Mesh::UploadMeshData(QueuedDeviceTransfer& transfer)
    {
        assert(memoryGroup_);
        memoryGroup_->FinalizeGroup();
        memoryGroup_->TransferData(transfer);
        vertexData_.clear();
    }

    void Mesh::BindBuffersToCommandBuffer(vk::CommandBuffer cmdBuffer) const
    {
        cmdBuffer.bindVertexBuffers(0, 1, vertexBuffer_.first->GetBufferPtr(), &vertexBuffer_.second);
        cmdBuffer.bindIndexBuffer(indexBuffer_.first->GetBuffer(), indexBuffer_.second, vk::IndexType::eUint32);
    }

    void Mesh::DrawMesh(vk::CommandBuffer cmdBuffer) const
    {
        // TODO: use submeshes and materials and the scene nodes...
        // need:
        // - for each submesh: 2 texture descriptors
        // - for each scene mesh node: uniform buffer?
        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(meshInfo_->GetIndices().size()), 1, 0, 0, 0);
    }

    void Mesh::SetVertexBuffer(const DeviceBuffer* vtxBuffer, std::size_t offset)
    {
        vertexBuffer_.first = vtxBuffer;
        vertexBuffer_.second = offset;
    }

    void Mesh::SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset)
    {
        indexBuffer_.first = idxBuffer;
        indexBuffer_.second = offset;
    }
}
