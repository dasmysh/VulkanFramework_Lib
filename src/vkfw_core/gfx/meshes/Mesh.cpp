/**
 * @file   Mesh.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.24
 *
 * @brief  Implementation of the Mesh class.
 */

#include "gfx/meshes/Mesh.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/Texture2D.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gfx/renderer/RenderList.h"
#include "gfx/camera/CameraBase.h"
#include "core/math/math.h"

namespace vku::gfx {

    Mesh::Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
        std::size_t numBackbuffers, const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        meshInfo_{ meshInfo },
        internalMemoryGroup_{ std::make_unique<MemoryGroup>(device, memoryFlags) },
        memoryGroup_{ internalMemoryGroup_.get() },
        bufferIdx_{ DeviceMemoryGroup::INVALID_INDEX },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        worldMatricesUBO_{ vku::gfx::UniformBufferObject::Create<WorldMatrixUBO>(device, numBackbuffers * meshInfo->GetNodes().size()) },
        materialsUBO_{ std::move(materialsUBO) }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
        std::size_t numBackbuffers, MemoryGroup& memoryGroup, unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        meshInfo_{ meshInfo },
        memoryGroup_{ &memoryGroup },
        bufferIdx_{ bufferIndex },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        worldMatricesUBO_{ vku::gfx::UniformBufferObject::Create<WorldMatrixUBO>(device, numBackbuffers * meshInfo->GetNodes().size()) },
        materialsUBO_{ std::move(materialsUBO) }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept :
        device_{ rhs.device_ },
        meshInfo_{ std::move(rhs.meshInfo_) },
        internalMemoryGroup_{ std::move(rhs.internalMemoryGroup_) },
        memoryGroup_{ rhs.memoryGroup_ },
        bufferIdx_{ rhs.bufferIdx_ },
        vertexBuffer_{ std::move(rhs.vertexBuffer_) },
        indexBuffer_{ std::move(rhs.indexBuffer_) },
        worldMatricesUBO_{ std::move(rhs.worldMatricesUBO_) },
        materialsUBO_{ std::move(rhs.materialsUBO_) },
        materials_{ std::move(rhs.materials_) },
        textureSampler_{ std::move(rhs.textureSampler_) },
        descriptorPool_{ std::move(rhs.descriptorPool_) },
        materialTexturesDescriptorSetLayout_{ std::move(rhs.materialTexturesDescriptorSetLayout_) },
        materialTextureDescriptorSets_{ std::move(rhs.materialTextureDescriptorSets_) },
        vertexMaterialData_{ std::move(rhs.vertexMaterialData_) }
    {
    }

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept
    {
        this->~Mesh();
        device_ = rhs.device_;
        meshInfo_ = std::move(rhs.meshInfo_);
        internalMemoryGroup_ = std::move(rhs.internalMemoryGroup_);
        memoryGroup_ = rhs.memoryGroup_;
        bufferIdx_ = rhs.bufferIdx_;
        vertexBuffer_ = std::move(rhs.vertexBuffer_);
        indexBuffer_ = std::move(rhs.indexBuffer_);
        worldMatricesUBO_ = std::move(rhs.worldMatricesUBO_);
        materialsUBO_ = std::move(rhs.materialsUBO_);
        materials_ = std::move(rhs.materials_);
        textureSampler_ = std::move(rhs.textureSampler_);
        descriptorPool_ = std::move(rhs.descriptorPool_);
        materialTexturesDescriptorSetLayout_ = std::move(rhs.materialTexturesDescriptorSetLayout_);
        materialTextureDescriptorSets_ = std::move(rhs.materialTextureDescriptorSets_);
        vertexMaterialData_ = std::move(rhs.vertexMaterialData_);
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        // sets:
        // 0: world
        // 1: material UBO
        // 2: material textures
        // 3: camera
        {
            vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
                vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat };
            textureSampler_ = device_->GetDevice().createSamplerUnique(samplerCreateInfo);
        }

        {
            // Shared material descriptor set layout
            std::vector<vk::DescriptorSetLayoutBinding> materialSetLayoutBindings;
            // Binding 1: Diffuse map
            materialSetLayoutBindings.emplace_back(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
            // Binding 2: Bump map
            materialSetLayoutBindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

            vk::DescriptorSetLayoutCreateInfo materialDescriptorLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(materialSetLayoutBindings.size()), materialSetLayoutBindings.data() };

            materialTexturesDescriptorSetLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(materialDescriptorLayoutCreateInfo);
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
            poolSizes.emplace_back(vk::DescriptorType::eUniformBufferDynamic, 2);
            poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, static_cast<std::uint32_t>(numMaterials * 2));

            vk::DescriptorPoolCreateInfo descriptorPoolInfo{ vk::DescriptorPoolCreateFlags(), static_cast<std::uint32_t>(numMaterials * 2 + 2),
                static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data() };

            descriptorPool_ = device_->GetDevice().createDescriptorPoolUnique(descriptorPoolInfo);
        }

        worldMatricesUBO_.CreateLayout(*descriptorPool_, vk::ShaderStageFlagBits::eVertex, true, 0);
        materialsUBO_.CreateLayout(*descriptorPool_, vk::ShaderStageFlagBits::eFragment, true, 0);

        std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(3 * numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorImageInfo> descImageInfos; descImageInfos.reserve(materials_.size() * 2);

        {
            std::vector<vk::DescriptorSetLayout> materialDescSetLayouts; materialDescSetLayouts.resize(numMaterials);

            for (auto& materialDescSetLayout : materialDescSetLayouts) {
                materialDescSetLayout = *materialTexturesDescriptorSetLayout_;
            }

            vk::DescriptorSetAllocateInfo materialDescSetAllocInfo{ *descriptorPool_, static_cast<std::uint32_t>(materialDescSetLayouts.size()), materialDescSetLayouts.data() };
            materialTextureDescriptorSets_ = device_->GetDevice().allocateDescriptorSets(materialDescSetAllocInfo);

            materialsUBO_.FillDescriptorSetWrite(descSetWrites.emplace_back());

            for (std::size_t i = 0; i < materials_.size(); ++i) {
                if (materials_[i].diffuseTexture_) {
                    descImageInfos.emplace_back(*textureSampler_,
                                                materials_[i].diffuseTexture_->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                } else {
                    descImageInfos.emplace_back(*textureSampler_,
                                                device_->GetDummyTexture()->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                }

                descSetWrites.emplace_back(materialTextureDescriptorSets_[i], 0, 0, 1,
                                           vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());

                if (materials_[i].bumpMap_) {
                    descImageInfos.emplace_back(*textureSampler_, materials_[i].bumpMap_->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                } else {
                    descImageInfos.emplace_back(*textureSampler_,
                                                device_->GetDummyTexture()->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                }

                descSetWrites.emplace_back(materialTextureDescriptorSets_[i], 1, 0, 1,
                                           vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());
            }
        }

        worldMatricesUBO_.FillDescriptorSetWrite(descSetWrites.emplace_back());

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
        for (const auto& node : meshInfo_->GetNodes()) {
            auto i = backbufferIdx * meshInfo_->GetNodes().size() + node->GetNodeIndex();
            worldMatricesUBO_.FillUploadCmdBuffer<WorldMatrixUBO>(transferCmdBuffer, i);
        }
    }

    void Mesh::UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const
    {
        UpdateWorldMatricesNode(backbufferIndex, meshInfo_->GetRootNode(), worldMatrix);
    }

    void Mesh::UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const
    {
        if (!node->HasMeshes()) { return; }

        auto nodeWorld = worldMatrix * node->GetLocalTransform();

        WorldMatrixUBO worldMatrices{glm::mat4{1.0f}, glm::mat4{1.0f}};
        worldMatrices.model_ = nodeWorld;
        worldMatrices.normalMatrix_ = glm::mat4(glm::inverseTranspose(glm::mat3(nodeWorld)));

        auto instanceIndex = backbufferIndex * meshInfo_->GetNodes().size() + node->GetNodeIndex();
        worldMatricesUBO_.UpdateInstanceData(instanceIndex, worldMatrices);

        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            UpdateWorldMatricesNode(backbufferIndex, node->GetChild(i), nodeWorld);
        }
    }

    void Mesh::Draw(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout) const
    {
        cmdBuffer.bindVertexBuffers(0, 1, vertexBuffer_.first->GetBufferPtr(), &vertexBuffer_.second);
        cmdBuffer.bindIndexBuffer(indexBuffer_.first->GetBuffer(), indexBuffer_.second, vk::IndexType::eUint32);

        DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, meshInfo_->GetRootNode());
    }

    void Mesh::DrawNode(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout,
        const SceneMeshNode* node) const
    {
        if (!node->HasMeshes()) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * meshInfo_->GetNodes().size() + node->GetNodeIndex();
        worldMatricesUBO_.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, instanceIndex);

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            DrawSubMesh(cmdBuffer, pipelineLayout, meshInfo_->GetSubMeshes()[node->GetSubMeshID(i)]);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, node->GetChild(i));
        }
    }

    void Mesh::DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh& subMesh) const
    {
        // bind material.
        auto& matDescSets = materialTextureDescriptorSets_[subMesh.GetMaterialID()];
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, matDescSets, nullptr);
        materialsUBO_.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, subMesh.GetMaterialID());

        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1, static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0);
    }

    void Mesh::GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx, RenderList& renderList) const
    {
        renderList.SetCurrentGeometry(vertexBuffer_, indexBuffer_);

        GetDrawElementsNode(worldMatrix, camera, backbufferIdx, meshInfo_->GetRootNode(), renderList);
    }

    void Mesh::GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera,
        std::size_t backbufferIdx, const SceneMeshNode* node, RenderList& renderList) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;

        auto aabb = node->GetBoundingBox().NewFromTransform(nodeWorld);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * meshInfo_->GetNodes().size() + node->GetNodeIndex();
        renderList.SetCurrentWorldMatrices(RenderElement::UBOBinding{ &worldMatricesUBO_, 0, instanceIndex });

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            GetDrawElementsSubMesh(nodeWorld, camera, meshInfo_->GetSubMeshes()[node->GetSubMeshID(i)], renderList);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            GetDrawElementsNode(nodeWorld, camera, backbufferIdx, node->GetChild(i), renderList);
        }
    }

    void Mesh::GetDrawElementsSubMesh(const glm::mat4& worldMatrix, const CameraBase& camera,
        const SubMesh& subMesh, RenderList& renderList) const
    {
        auto aabb = subMesh.GetLocalAABB().NewFromTransform(worldMatrix);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }


        // bind material.
        const auto mat = meshInfo_->GetMaterial(subMesh.GetMaterialID());
        auto hasTransparency = (mat->alpha_ < 1.0f) || mat->hasAlpha_;

        auto& matDescSets = materialTextureDescriptorSets_[subMesh.GetMaterialID()];
        RenderElement::DescSetBinding materialTextureBinding(matDescSets, 2);
        RenderElement::UBOBinding materialBinding(&materialsUBO_, 1, subMesh.GetMaterialID());

        RenderElement* re = nullptr;
        if (hasTransparency) {
            re = &renderList.AddTransparentElement(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1,
                                                   static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0,
                                                   camera.GetViewMatrix(), aabb);
        } else {
            re = &renderList.AddOpaqueElement(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1,
                                              static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0,
                                              camera.GetViewMatrix(), aabb);
        }

        re->BindUBO(materialBinding);
        re->BindDescriptorSet(materialTextureBinding);
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
