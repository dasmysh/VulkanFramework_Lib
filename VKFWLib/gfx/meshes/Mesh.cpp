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
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vku::gfx {

    Mesh::Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        vk::MemoryPropertyFlags memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        meshInfo_{ meshInfo },
        internalMemoryGroup_{ std::make_unique<MemoryGroup>(device, memoryFlags) },
        memoryGroup_{ internalMemoryGroup_.get() },
        bufferIdx_{ DeviceMemoryGroup::INVALID_INDEX },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        materialBuffer_{ nullptr, 0, 0 }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        meshInfo_{ meshInfo },
        memoryGroup_{ &memoryGroup },
        bufferIdx_{ bufferIndex },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        materialBuffer_{ nullptr, 0, 0 }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept :
        device_{ std::move(rhs.device_) },
        meshInfo_{ std::move(rhs.meshInfo_) },
        internalMemoryGroup_{ std::move(rhs.internalMemoryGroup_) },
        memoryGroup_{ std::move(rhs.memoryGroup_) },
        bufferIdx_{ std::move(rhs.bufferIdx_) },
        vertexBuffer_{ std::move(rhs.vertexBuffer_) },
        indexBuffer_{ std::move(rhs.indexBuffer_) },
        materialBuffer_{ std::move(rhs.materialBuffer_) },
        worldMatricesBuffer_{ std::move(rhs.worldMatricesBuffer_) },
        materials_{ std::move(rhs.materials_) },
        textureSampler_{ std::move(rhs.textureSampler_) },
        descriptorPool_{ std::move(rhs.descriptorPool_) },
        materialDescriptorSetLayout_{ std::move(rhs.materialDescriptorSetLayout_) },
        materialDescriptorSets_{ std::move(rhs.materialDescriptorSets_) },
        worldMatricesDescriptorSetLayout_{ std::move(rhs.worldMatricesDescriptorSetLayout_) },
        worldMatricesDescriptorSets_{ std::move(rhs.worldMatricesDescriptorSets_) },
        vertexMaterialData_{ std::move(rhs.vertexMaterialData_) }
    {
    }

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept
    {
        this->~Mesh();
        device_ = std::move(rhs.device_);
        meshInfo_ = std::move(rhs.meshInfo_);
        internalMemoryGroup_ = std::move(rhs.internalMemoryGroup_);
        memoryGroup_ = std::move(rhs.memoryGroup_);
        bufferIdx_ = std::move(rhs.bufferIdx_);
        vertexBuffer_ = std::move(rhs.vertexBuffer_);
        indexBuffer_ = std::move(rhs.indexBuffer_);
        materialBuffer_ = std::move(rhs.materialBuffer_);
        worldMatricesBuffer_ = std::move(rhs.worldMatricesBuffer_);
        materials_ = std::move(rhs.materials_);
        textureSampler_ = std::move(rhs.textureSampler_);
        descriptorPool_ = std::move(rhs.descriptorPool_);
        materialDescriptorSetLayout_ = std::move(rhs.materialDescriptorSetLayout_);
        materialDescriptorSets_ = std::move(rhs.materialDescriptorSets_);
        worldMatricesDescriptorSetLayout_ = std::move(rhs.worldMatricesDescriptorSetLayout_);
        worldMatricesDescriptorSets_ = std::move(rhs.worldMatricesDescriptorSets_);
        vertexMaterialData_ = std::move(rhs.vertexMaterialData_);
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        {
            vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
                vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat };
            textureSampler_ = device_->GetDevice().createSamplerUnique(samplerCreateInfo);
        }

        {
            // Shared material descriptor set layout
            std::vector<vk::DescriptorSetLayoutBinding> materialSetLayoutBindings;
            // Binding 0: Material UBO
            materialSetLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
            // Binding 1: Diffuse map
            materialSetLayoutBindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
            // Binding 2: Bump map
            materialSetLayoutBindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

            vk::DescriptorSetLayoutCreateInfo materialDescriptorLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(materialSetLayoutBindings.size()), materialSetLayoutBindings.data() };

            materialDescriptorSetLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(materialDescriptorLayoutCreateInfo);
        }

        {
            // Shared world matrices descriptor set layout
            std::vector<vk::DescriptorSetLayoutBinding> worldMatricesSetLayoutBindings;
            // Binding 1: Local Matrices UBO
            worldMatricesSetLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

            vk::DescriptorSetLayoutCreateInfo worldMatricesDescriptorLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(worldMatricesSetLayoutBindings.size()), worldMatricesSetLayoutBindings.data() };

            worldMatricesDescriptorSetLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(worldMatricesDescriptorLayoutCreateInfo);
        }

        materials_.reserve(meshInfo_->GetMaterials().size());
        for (const auto& mat : meshInfo_->GetMaterials()) {
            materials_.emplace_back(&mat, device_, *memoryGroup_, queueFamilyIndices);
        }
    }

    void Mesh::CreateDescriptorSets(std::size_t numBackbuffers)
    {
        auto numMaterials = meshInfo_->GetMaterials().size();
        auto numNodes = meshInfo_->GetNodes().size();
        {
            std::vector<vk::DescriptorPoolSize> poolSizes;
            poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, static_cast<std::uint32_t>(numMaterials + numNodes * numBackbuffers));
            poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, static_cast<std::uint32_t>(numMaterials * 2));

            vk::DescriptorPoolCreateInfo descriptorPoolInfo{ vk::DescriptorPoolCreateFlags(), static_cast<std::uint32_t>(numMaterials * 3),
                static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data() };

            descriptorPool_ = device_->GetDevice().createDescriptorPoolUnique(descriptorPoolInfo);
        }

        std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(3 * numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorImageInfo> descImageInfos; descImageInfos.reserve(materials_.size() * 2);

        {
            std::vector<vk::DescriptorSetLayout> materialDescSetLayouts; materialDescSetLayouts.resize(numMaterials);

            for (auto& materialDescSetLayout : materialDescSetLayouts) {
                materialDescSetLayout = *materialDescriptorSetLayout_;
            }

            vk::DescriptorSetAllocateInfo materialDescSetAllocInfo{ *descriptorPool_, static_cast<std::uint32_t>(materialDescSetLayouts.size()), materialDescSetLayouts.data() };
            materialDescriptorSets_ = device_->GetDevice().allocateDescriptorSets(materialDescSetAllocInfo);

            auto uboOffset = static_cast<std::uint32_t>(std::get<1>(materialBuffer_));
            auto materialBufferSize = static_cast<std::uint32_t>(std::get<2>(materialBuffer_));
            for (std::size_t i = 0; i < materials_.size(); ++i) {
                auto materialBufferOffset = uboOffset + materialBufferSize * i;
                descBufferInfos.emplace_back(std::get<0>(materialBuffer_)->GetBuffer(), materialBufferOffset, materialBufferSize);
                descSetWrites.emplace_back(materialDescriptorSets_[i], 0, 0, 1,
                    vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos[i]);

                if (materials_[i].diffuseTexture_) descImageInfos.emplace_back(*textureSampler_, materials_[i].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
                else descImageInfos.emplace_back(*textureSampler_, device_->GetDummyTexture()->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);

                descSetWrites.emplace_back(materialDescriptorSets_[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());

                if (materials_[i].bumpMap_) descImageInfos.emplace_back(*textureSampler_, materials_[i].bumpMap_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
                else descImageInfos.emplace_back(*textureSampler_, device_->GetDummyTexture()->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);

                descSetWrites.emplace_back(materialDescriptorSets_[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());
            }
        }

        {
            std::vector<vk::DescriptorSetLayout> worldMatDescSetLayouts; worldMatDescSetLayouts.resize(numBackbuffers * numNodes);

            for (auto& worldMatDescSetLayout : worldMatDescSetLayouts) {
                worldMatDescSetLayout = *worldMatricesDescriptorSetLayout_;
            }

            vk::DescriptorSetAllocateInfo worldMatDescSetAllocInfo{ *descriptorPool_, static_cast<std::uint32_t>(worldMatDescSetLayouts.size()), worldMatDescSetLayouts.data() };
            worldMatricesDescriptorSets_ = device_->GetDevice().allocateDescriptorSets(worldMatDescSetAllocInfo);

            auto uboOffset = static_cast<std::uint32_t>(std::get<1>(worldMatricesBuffer_));
            auto worldMatricesBufferSize = static_cast<std::uint32_t>(std::get<3>(worldMatricesBuffer_));
            for (std::size_t ibb = 0; ibb < numBackbuffers; ++ibb) {
                for (std::size_t in = 0; in < numNodes; ++in) {
                    auto i = ibb * numNodes + in;
                    auto worldMatricesBufferOffset = uboOffset + worldMatricesBufferSize * i;
                    descBufferInfos.emplace_back(std::get<0>(worldMatricesBuffer_)->GetBuffer(), worldMatricesBufferOffset, worldMatricesBufferSize);
                    descSetWrites.emplace_back(worldMatricesDescriptorSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos.back());
                }
            }
        }

        device_->GetDevice().updateDescriptorSets(descSetWrites, nullptr);
    }

    void Mesh::UploadMeshData(QueuedDeviceTransfer& transfer)
    {
        assert(memoryGroup_);
        memoryGroup_->FinalizeDeviceGroup();
        memoryGroup_->TransferData(transfer);
        vertexMaterialData_.clear();
    }

    void Mesh::TransferWorldMatrices(vk::CommandBuffer transferCmdBuffer, std::size_t backbufferIdx) const
    {
        auto worldUBOOffset = std::get<1>(worldMatricesBuffer_) + (backbufferIdx * std::get<2>(worldMatricesBuffer_));
        memoryGroup_->FillUploadBufferCmdBuffer(bufferIdx_, transferCmdBuffer, worldUBOOffset, std::get<2>(worldMatricesBuffer_));
    }

    void Mesh::UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const
    {
        auto meshWorld = meshInfo_->GetRootTransform() * worldMatrix;
        UpdateWorldMatricesNode(backbufferIndex, meshInfo_->GetRootNode(), meshWorld);
    }

    void Mesh::UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;

        std::array<glm::mat4, 2> worldMatrices;
        worldMatrices[0] = nodeWorld;
        worldMatrices[1] = glm::mat4(glm::inverseTranspose(glm::mat3(nodeWorld)));

        auto offset = memoryGroup_->GetHostBufferOffset(bufferIdx_) + std::get<1>(worldMatricesBuffer_)
            + std::get<2>(worldMatricesBuffer_) * backbufferIndex + std::get<3>(worldMatricesBuffer_) * node->GetNodeIndex();
        
        memoryGroup_->GetHostMemory()->CopyToHostMemory(offset, 2 * sizeof(glm::mat4), worldMatrices.data());

        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) UpdateWorldMatricesNode(backbufferIndex, node->GetChild(i), nodeWorld);
    }

    void Mesh::Draw(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout) const
    {
        cmdBuffer.bindVertexBuffers(0, 1, vertexBuffer_.first->GetBufferPtr(), &vertexBuffer_.second);
        cmdBuffer.bindIndexBuffer(indexBuffer_.first->GetBuffer(), indexBuffer_.second, vk::IndexType::eUint32);

        // TODO: add depth sorting here...
        DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, meshInfo_->GetRootNode());

        // TODO:
        // need:
        // - dynamic ubos.
    }

    void Mesh::DrawNode(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout,
        const SceneMeshNode* node) const
    {
        // bind world matrices
        auto& worldMatDescSets = worldMatricesDescriptorSets_[backbufferIdx * meshInfo_->GetNodes().size() + node->GetNodeIndex()];
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, worldMatDescSets, nullptr);

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) DrawSubMesh(cmdBuffer, pipelineLayout, meshInfo_->GetSubMesh(node->GetSubMeshID(i)));
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, node->GetChild(i));
    }

    void Mesh::DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh* subMesh) const
    {
        // bind material.
        auto& matDescSets = materialDescriptorSets_[subMesh->GetMaterialID()];
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, matDescSets, nullptr);

        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(subMesh->GetNumberOfIndices()), 1, static_cast<std::uint32_t>(subMesh->GetIndexOffset()), 0, 0);
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

    void Mesh::SetMaterialBuffer(const DeviceBuffer* matBuffer, std::size_t offset, std::size_t elementSize)
    {
        materialBuffer_ = std::make_tuple(matBuffer, offset, elementSize);
    }

    void Mesh::SetWorldMatricesBuffer(const DeviceBuffer* worldMatricesBuffer, std::size_t offset, std::size_t bufferSize, std::size_t elementSize)
    {
        worldMatricesBuffer_ = std::make_tuple(worldMatricesBuffer, offset, bufferSize, elementSize);
    }
}
