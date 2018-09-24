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
        vertexMaterialData_{ std::move(rhs.vertexMaterialData_) }
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
        vertexMaterialData_ = std::move(rhs.vertexMaterialData_);
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const LogicalDevice* device, MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto numMaterials = meshInfo_->GetMaterials().size();
        std::vector<vk::DescriptorPoolSize> poolSizes;
        poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, numMaterials);
        poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, numMaterials * 2);

        vk::DescriptorPoolCreateInfo descriptorPoolInfo{ vk::DescriptorPoolCreateFlags(), numMaterials * 3,
            static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data() };

        auto vkUBODescriptorPool_ = device->GetDevice().createDescriptorPool(descriptorPoolInfo);


        // Shared descriptor set layout
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
        // Binding 0: UBO
        setLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
        // Binding 1: Diffuse map
        setLayoutBindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
        // Binding 1: Bump map
        setLayoutBindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

        vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(), setLayoutBindings.size(), setLayoutBindings.data() };

        auto descSetLayout = device->GetDevice().createDescriptorSetLayout(descriptorLayoutCreateInfo);


        std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(numMaterials);
        std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(numMaterials);
        std::vector<vk::DescriptorBufferInfo> descWriteInfos; descWriteInfos.reserve(numMaterials * 2);

        materials_.reserve(numMaterials);
        for (std::size_t i = 0; i < numMaterials; ++i) {
            const auto& mat = meshInfo_->GetMaterials()[i];
            materials_.emplace_back(&mat, device, memoryGroup, queueFamilyIndices);
            descSetLayouts[i] = descSetLayout;
        }

        vk::DescriptorSetAllocateInfo descSetAllocInfo{ vkUBODescriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
        auto vkUBOSamplerDescritorSets_ = device->GetDevice().allocateDescriptorSets(descSetAllocInfo);

        for (std::size_t i = 0; i < numMaterials; ++i) {
            descSetWrites.emplace_back(memGroup_.GetBuffer(completeBufferIdx_)->GetBuffer(), bufferOffset, singleUBOSize);
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[i], 0, i, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufferInfos[i], nullptr);

            vk::DescriptorImageInfo descImageInfo{ vkDemoSampler_, materials_[i].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[i], 1, i, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfo);
            vk::DescriptorImageInfo descImageInfo{ vkDemoSampler_, materials_[i].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
            descSetWrites.emplace_back(vkUBOSamplerDescritorSets_[i], 2, i, 1, vk::DescriptorType::eCombinedImageSampler, &descImageInfo);
        }

        device.GetDevice().updateDescriptorSets(descSetWrites, nullptr);


        // {
        //     vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(),
        //         static_cast<std::uint32_t>(vkDescriptorSetLayouts_.size()), vkDescriptorSetLayouts_.data(), 0, nullptr };
        //     vkPipelineLayout_ = device.GetDevice().createPipelineLayout(pipelineLayoutInfo);
        // }

        {
            std::vector<vk::DescriptorSetLayout> descSetLayouts; descSetLayouts.resize(numUBOBuffers + 1);
            descSetLayouts[0] = descSetLayout;
            for (auto i = 0U; i < numUBOBuffers; ++i) descSetLayouts[i + 1] = vkDescriptorSetLayouts_[1];
            vk::DescriptorSetAllocateInfo descSetAllocInfo{ vkUBODescriptorPool_, static_cast<std::uint32_t>(descSetLayouts.size()), descSetLayouts.data() };
            vkUBOSamplerDescritorSets_ = device.GetDevice().allocateDescriptorSets(descSetAllocInfo);
        }
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

    void Mesh::DrawMesh(vk::CommandBuffer cmdBuffer, const glm::mat4& worldMatrix) const
    {
        auto meshWorld = meshInfo_->GetRootTransform() * worldMatrix;
        DrawMeshNode(cmdBuffer, meshInfo_->GetRootNode(), meshWorld);

        // TODO: use submeshes and materials and the scene nodes...
        // need:
        // - for each submesh: 2 texture descriptors
        // - for each scene mesh node: uniform buffer?
        // - uniform buffer for all materials[array] (via template)
        // - push constant for world matrix
        // - push constant for material index .. or "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"
        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(meshInfo_->GetIndices().size()), 1, 0, 0, 0);
    }

    void Mesh::DrawMeshNode(vk::CommandBuffer cmdBuffer, const SceneMeshNode* node, const glm::mat4& worldMatrix) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;
        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) DrawSubMesh(cmdBuffer, meshInfo_->GetSubMesh(node->GetSubMeshID(i)), nodeWorld);
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) DrawMeshNode(cmdBuffer, node->GetChild(i), nodeWorld);
    }

    void Mesh::DrawSubMesh(vk::CommandBuffer cmdBuffer, const SubMesh* subMesh, const glm::mat4& worldMatrix) const
    {
        glUniformMatrix4fv(uniformLocations_[0], 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(uniformLocations_[1], 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(modelMatrix))));

        auto mat = mesh_->GetMaterial(subMesh->GetMaterialIndex());
        auto matTex = mesh_->GetMaterialTexture(subMesh->GetMaterialIndex());
        if (matTex->diffuseTex && uniformLocations_.size() > 2) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, matTex->diffuseTex->getTextureId());
            glUniform1i(uniformLocations_[2], 0);
        }
        if (matTex->bumpTex && uniformLocations_.size() > 3) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, matTex->bumpTex->getTextureId());
            glUniform1i(uniformLocations_[3], 1);
            if (!overrideBump) glUniform1f(uniformLocations_[4], mat->bumpMultiplier);
        }

        glDrawElements(GL_TRIANGLES, subMesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
            (static_cast<char*> (nullptr)) + (static_cast<std::size_t>(subMesh->GetIndexOffset()) * sizeof(unsigned int)));



        auto worldMat = worldMatrix;
        auto normalMat = glm::inverseTranspose(glm::mat3(worldMatrix));
        auto& mat = materials_[subMesh->GetMaterialID()];
        auto& matDescSets = materialDescriptorSets_[subMesh->GetMaterialID()];

        // bind material ubo.
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout_, 0, matDescSets.materialUBODescriptorSet_, nullptr);
        if (mat.diffuseTexture_) {
            // bind sampler for diffuse texture.
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout_, 0, matDescSets.diffuseTexDescriptorSet_, nullptr);
        }

        if (mat.bumpMap_) {
            // bind sampler for bump texture.
            cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout_, 0, matDescSets.bumpTexDescriptorSet_, nullptr);
        }

        // TODO: Set material, set texture(s). [3/26/2017 Sebastian Maisch]
        // use a single descriptor set with dynamic offsets for the material
        // actually push constants do not work if the command buffer stays constant for multiple frames.
        cmdBuffer.pushConstants(vkPipelineLayout_, vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<float>(16, glm::value_ptr(worldMat)));
        cmdBuffer.pushConstants(vkPipelineLayout_, vk::ShaderStageFlagBits::eVertex, 16, vk::ArrayProxy<float>(9, glm::value_ptr(normalMat)));
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

    void Mesh::SetMaterialBuffer(const DeviceBuffer* matBuffer, std::size_t offset)
    {
        materialBuffer_.first = matBuffer;
        materialBuffer_.second = offset;
    }
}
