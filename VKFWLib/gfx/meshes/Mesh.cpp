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
        memoryGroup_{ std::make_unique<MemoryGroup>(device, memoryFlags) },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        materialBuffer_{ nullptr, 0, 0 }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(std::shared_ptr<const MeshInfo> meshInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        meshInfo_{ meshInfo },
        vertexBuffer_{ nullptr, 0 },
        indexBuffer_{ nullptr, 0 },
        materialBuffer_{ nullptr, 0, 0 }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept :
        device_{ std::move(rhs.device_) },
        meshInfo_{ std::move(rhs.meshInfo_) },
        memoryGroup_{ std::move(rhs.memoryGroup_) },
        vertexBuffer_{ std::move(rhs.vertexBuffer_) },
        indexBuffer_{ std::move(rhs.indexBuffer_) },
        materialBuffer_{ std::move(rhs.materialBuffer_) },
        materials_{ std::move(rhs.materials_) },
        textureSampler_{ std::move(rhs.textureSampler_) },
        descriptorPool_{ std::move(rhs.descriptorPool_) },
        descriptorSetLayout_{ std::move(rhs.descriptorSetLayout_) },
        materialDescriptorSets_{ std::move(rhs.materialDescriptorSets_) },
        vertexMaterialData_{ std::move(rhs.vertexMaterialData_) }
    {
    }

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept
    {
        this->~Mesh();
        device_ = std::move(rhs.device_);
        meshInfo_ = std::move(rhs.meshInfo_);
        memoryGroup_ = std::move(rhs.memoryGroup_);
        vertexBuffer_ = std::move(rhs.vertexBuffer_);
        indexBuffer_ = std::move(rhs.indexBuffer_);
        materialBuffer_ = std::move(rhs.materialBuffer_);
        materials_ = std::move(rhs.materials_);
        textureSampler_ = std::move(rhs.textureSampler_);
        descriptorPool_ = std::move(rhs.descriptorPool_);
        descriptorSetLayout_ = std::move(rhs.descriptorSetLayout_);
        materialDescriptorSets_ = std::move(rhs.materialDescriptorSets_);
        vertexMaterialData_ = std::move(rhs.vertexMaterialData_);
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat };
        textureSampler_ = device_->GetDevice().createSamplerUnique(samplerCreateInfo);

        auto numMaterials = meshInfo_->GetMaterials().size();
        std::vector<vk::DescriptorPoolSize> poolSizes;
        poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, static_cast<std::uint32_t>(numMaterials));
        poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, static_cast<std::uint32_t>(numMaterials * 2));

        vk::DescriptorPoolCreateInfo descriptorPoolInfo{ vk::DescriptorPoolCreateFlags(), static_cast<std::uint32_t>(numMaterials * 3),
            static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data() };

        descriptorPool_ = device_->GetDevice().createDescriptorPoolUnique(descriptorPoolInfo);

        // Shared descriptor set layout
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
        // Binding 0: UBO
        setLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
        // Binding 1: Diffuse map
        setLayoutBindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
        // Binding 1: Bump map
        // setLayoutBindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

        vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
            static_cast<std::uint32_t>(setLayoutBindings.size()), setLayoutBindings.data() };

        descriptorSetLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(descriptorLayoutCreateInfo);

        materials_.reserve(numMaterials);
        for (std::size_t i = 0; i < numMaterials; ++i) {
            const auto& mat = meshInfo_->GetMaterials()[i];
            materials_.emplace_back(&mat, device_, *memoryGroup_, queueFamilyIndices);
        }

        // {
        //     vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(),
        //         static_cast<std::uint32_t>(vkDescriptorSetLayouts_.size()), vkDescriptorSetLayouts_.data(), 0, nullptr };
        //     vkPipelineLayout_ = device.GetDevice().createPipelineLayout(pipelineLayoutInfo);
        // }

        // {
        //     std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(numUBOBuffers + 1);
        //     descSetLayouts[0] = descSetLayout;
        //     for (auto i = 0U; i < numUBOBuffers; ++i) descSetLayouts[i + 1] = vkDescriptorSetLayouts_[1];
        //     vk::DescriptorSetAllocateInfo descSetAllocInfo{ vkUBODescriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
        //     vkUBOSamplerDescritorSets_ = device.GetDevice().allocateDescriptorSets(descSetAllocInfo);
        // }
    }

    void Mesh::CreateDescriptorSets()
    {
        std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(materials_.size());
        std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(materials_.size());
        std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(materials_.size());
        std::vector<vk::DescriptorImageInfo> descImageInfos; descImageInfos.reserve(materials_.size() * 2);

        for (std::size_t i = 0; i < materials_.size(); ++i) {
            descSetLayouts[i] = *descriptorSetLayout_;
        }

        vk::DescriptorSetAllocateInfo descSetAllocInfo{ *descriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
        materialDescriptorSets_ = device_->GetDevice().allocateDescriptorSetsUnique(descSetAllocInfo);

        auto uboOffset = static_cast<std::uint32_t>(std::get<1>(materialBuffer_));
        auto materialBufferSize = static_cast<std::uint32_t>(std::get<2>(materialBuffer_));
        for (std::size_t i = 0; i < materials_.size(); ++i) {
            auto materialBufferOffset = uboOffset + materialBufferSize * i;
            descBufferInfos.emplace_back(std::get<0>(materialBuffer_)->GetBuffer(), materialBufferOffset, materialBufferSize);
            //descSetWrites.emplace_back(*materialDescriptorSets_[i], 0, static_cast<std::uint32_t>(i), 1,
            //    vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos[i]);
            descSetWrites.emplace_back(*materialDescriptorSets_[i], 0, 0, 1,
                vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos[i]);

            if (materials_[i].diffuseTexture_) {
                descImageInfos.emplace_back(*textureSampler_, materials_[i].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
                // descSetWrites.emplace_back(*materialDescriptorSets_[i], 1, static_cast<std::uint32_t>(i), 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos[2 * i + 0]);
            
                descSetWrites.emplace_back(*materialDescriptorSets_[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos[i + 0]); // no bump map ...
            }
            else {
                descImageInfos.emplace_back(*textureSampler_, materials_[0].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
                // descSetWrites.emplace_back(*materialDescriptorSets_[i], 1, static_cast<std::uint32_t>(i), 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos[2 * i + 0]);
            
                descSetWrites.emplace_back(*materialDescriptorSets_[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos[i + 0]); // no bump map ...
                // descImageInfos.emplace_back(vk::Sampler(), vk::ImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
                // descSetWrites.emplace_back(materialDescriptorSets_[i], 1, static_cast<std::uint32_t>(i), 0, vk::DescriptorType::eCombinedImageSampler, nullptr);
            }

            // if (materials_[i].bumpMap_) {
            //     descImageInfos.emplace_back(*textureSampler_, materials_[i].bumpMap_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
            //     descSetWrites.emplace_back(*materialDescriptorSets_[i], 2, static_cast<std::uint32_t>(i), 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfos[2 * i + 1]);
            // }
            // else {
            //     // descImageInfos.emplace_back(vk::Sampler(), vk::ImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
            //     // descSetWrites.emplace_back(materialDescriptorSets_[i], 2, static_cast<std::uint32_t>(i), 0, vk::DescriptorType::eCombinedImageSampler, nullptr);
            // }
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

    void Mesh::BindBuffersToCommandBuffer(vk::CommandBuffer cmdBuffer) const
    {
        cmdBuffer.bindVertexBuffers(0, 1, vertexBuffer_.first->GetBufferPtr(), &vertexBuffer_.second);
        cmdBuffer.bindIndexBuffer(indexBuffer_.first->GetBuffer(), indexBuffer_.second, vk::IndexType::eUint32);
    }

    void Mesh::DrawMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const glm::mat4& worldMatrix) const
    {
        auto meshWorld = meshInfo_->GetRootTransform() * worldMatrix;
        // TODO: add depth sorting here...
        DrawMeshNode(cmdBuffer, pipelineLayout, meshInfo_->GetRootNode(), meshWorld);

        // TODO: use submeshes and materials and the scene nodes...
        // need:
        // - for each submesh: 2 texture descriptors
        // - for each scene mesh node: uniform buffer?
        // - uniform buffer for all materials[array] (via template)
        // - push constant for world matrix
        // - push constant for material index .. or "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"
        // cmdBuffer.drawIndexed(static_cast<std::uint32_t>(meshInfo_->GetIndices().size()), 1, 0, 0, 0);
    }

    void Mesh::DrawMeshNode(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout,
        const SceneMeshNode* node, const glm::mat4& worldMatrix) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) DrawSubMesh(cmdBuffer, pipelineLayout, meshInfo_->GetSubMesh(node->GetSubMeshID(i)), nodeWorld);
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) DrawMeshNode(cmdBuffer, pipelineLayout, node->GetChild(i), nodeWorld);
    }

    void Mesh::DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh* subMesh, const glm::mat4& worldMatrix) const
    {
        auto worldMat = worldMatrix;
        auto normalMat = glm::inverseTranspose(glm::mat3(worldMatrix));
        auto& mat = materials_[subMesh->GetMaterialID()];
        auto& matDescSets = materialDescriptorSets_[subMesh->GetMaterialID()];

        // bind material ubo.
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *matDescSets, nullptr);
        cmdBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const float>(16, glm::value_ptr(worldMat)));
        cmdBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 16, vk::ArrayProxy<const float>(9, glm::value_ptr(normalMat)));
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
}
