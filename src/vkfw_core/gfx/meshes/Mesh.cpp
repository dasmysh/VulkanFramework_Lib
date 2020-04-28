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

namespace vkfw_core::gfx {

    Mesh::Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
        std::size_t numBackbuffers, const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices) :
        m_device{ device },
        m_meshInfo{ meshInfo },
        m_internalMemoryGroup{ std::make_unique<MemoryGroup>(device, memoryFlags) },
        m_memoryGroup{ m_internalMemoryGroup.get() },
        m_bufferIdx{ DeviceMemoryGroup::INVALID_INDEX },
        m_vertexBuffer{ nullptr, 0 },
        m_indexBuffer{ nullptr, 0 },
        m_worldMatricesUBO{ vkfw_core::gfx::UniformBufferObject::Create<WorldMatrixUBO>(device, numBackbuffers * meshInfo->GetNodes().size()) },
        m_materialsUBO{ std::move(materialsUBO) }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(const LogicalDevice* device, const std::shared_ptr<const MeshInfo>& meshInfo, UniformBufferObject&& materialsUBO,
        std::size_t numBackbuffers, MemoryGroup& memoryGroup, unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices) :
        m_device{ device },
        m_meshInfo{ meshInfo },
        m_memoryGroup{ &memoryGroup },
        m_bufferIdx{ bufferIndex },
        m_vertexBuffer{ nullptr, 0 },
        m_indexBuffer{ nullptr, 0 },
        m_worldMatricesUBO{ vkfw_core::gfx::UniformBufferObject::Create<WorldMatrixUBO>(device, numBackbuffers * meshInfo->GetNodes().size()) },
        m_materialsUBO{ std::move(materialsUBO) }
    {
        CreateMaterials(queueFamilyIndices);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept :
        m_device{ rhs.m_device },
        m_meshInfo{ std::move(rhs.m_meshInfo) },
        m_internalMemoryGroup{ std::move(rhs.m_internalMemoryGroup) },
        m_memoryGroup{ rhs.m_memoryGroup },
        m_bufferIdx{ rhs.m_bufferIdx },
        m_vertexBuffer{ std::move(rhs.m_vertexBuffer) },
        m_indexBuffer{ std::move(rhs.m_indexBuffer) },
        m_worldMatricesUBO{ std::move(rhs.m_worldMatricesUBO) },
        m_materialsUBO{ std::move(rhs.m_materialsUBO) },
        m_materials{ std::move(rhs.m_materials) },
        m_textureSampler{ std::move(rhs.m_textureSampler) },
        m_descriptorPool{ std::move(rhs.m_descriptorPool) },
        m_materialTexturesDescriptorSetLayout{ std::move(rhs.m_materialTexturesDescriptorSetLayout) },
        m_materialTextureDescriptorSets{ std::move(rhs.m_materialTextureDescriptorSets) },
        m_vertexMaterialData{ std::move(rhs.m_vertexMaterialData) }
    {
    }

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept
    {
        this->~Mesh();
        m_device = rhs.m_device;
        m_meshInfo = std::move(rhs.m_meshInfo);
        m_internalMemoryGroup = std::move(rhs.m_internalMemoryGroup);
        m_memoryGroup = rhs.m_memoryGroup;
        m_bufferIdx = rhs.m_bufferIdx;
        m_vertexBuffer = std::move(rhs.m_vertexBuffer);
        m_indexBuffer = std::move(rhs.m_indexBuffer);
        m_worldMatricesUBO = std::move(rhs.m_worldMatricesUBO);
        m_materialsUBO = std::move(rhs.m_materialsUBO);
        m_materials = std::move(rhs.m_materials);
        m_textureSampler = std::move(rhs.m_textureSampler);
        m_descriptorPool = std::move(rhs.m_descriptorPool);
        m_materialTexturesDescriptorSetLayout = std::move(rhs.m_materialTexturesDescriptorSetLayout);
        m_materialTextureDescriptorSets = std::move(rhs.m_materialTextureDescriptorSets);
        m_vertexMaterialData = std::move(rhs.m_vertexMaterialData);
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
            m_textureSampler = m_device->GetDevice().createSamplerUnique(samplerCreateInfo);
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

            m_materialTexturesDescriptorSetLayout = m_device->GetDevice().createDescriptorSetLayoutUnique(materialDescriptorLayoutCreateInfo);
        }

        m_materials.reserve(m_meshInfo->GetMaterials().size());
        for (const auto& mat : m_meshInfo->GetMaterials()) {
            m_materials.emplace_back(&mat, m_device, *m_memoryGroup, queueFamilyIndices);
        }
    }

    void Mesh::CreateDescriptorSets(std::size_t numBackbuffers)
    {
        auto numMaterials = m_meshInfo->GetMaterials().size();
        auto numNodes = m_meshInfo->GetNodes().size();
        {
            std::vector<vk::DescriptorPoolSize> poolSizes;
            poolSizes.emplace_back(vk::DescriptorType::eUniformBufferDynamic, 2);
            poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, static_cast<std::uint32_t>(numMaterials * 2));

            vk::DescriptorPoolCreateInfo descriptorPoolInfo{ vk::DescriptorPoolCreateFlags(), static_cast<std::uint32_t>(numMaterials * 2 + 2),
                static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data() };

            m_descriptorPool = m_device->GetDevice().createDescriptorPoolUnique(descriptorPoolInfo);
        }

        m_worldMatricesUBO.CreateLayout(*m_descriptorPool, vk::ShaderStageFlagBits::eVertex, true, 0);
        m_materialsUBO.CreateLayout(*m_descriptorPool, vk::ShaderStageFlagBits::eFragment, true, 0);

        std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(3 * numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorBufferInfo> descBufferInfos; descBufferInfos.reserve(numMaterials + numNodes * numBackbuffers);
        std::vector<vk::DescriptorImageInfo> descImageInfos; descImageInfos.reserve(m_materials.size() * 2);

        {
            std::vector<vk::DescriptorSetLayout> materialDescSetLayouts; materialDescSetLayouts.resize(numMaterials);

            for (auto& materialDescSetLayout : materialDescSetLayouts) {
                materialDescSetLayout = *m_materialTexturesDescriptorSetLayout;
            }

            vk::DescriptorSetAllocateInfo materialDescSetAllocInfo{ *m_descriptorPool, static_cast<std::uint32_t>(materialDescSetLayouts.size()), materialDescSetLayouts.data() };
            m_materialTextureDescriptorSets = m_device->GetDevice().allocateDescriptorSets(materialDescSetAllocInfo);

            m_materialsUBO.FillDescriptorSetWrite(descSetWrites.emplace_back());

            for (std::size_t i = 0; i < m_materials.size(); ++i) {
                if (m_materials[i].m_diffuseTexture) {
                    descImageInfos.emplace_back(*m_textureSampler,
                                                m_materials[i].m_diffuseTexture->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                } else {
                    descImageInfos.emplace_back(*m_textureSampler,
                                                m_device->GetDummyTexture()->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                }

                descSetWrites.emplace_back(m_materialTextureDescriptorSets[i], 0, 0, 1,
                                           vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());

                if (m_materials[i].m_bumpMap) {
                    descImageInfos.emplace_back(*m_textureSampler, m_materials[i].m_bumpMap->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                } else {
                    descImageInfos.emplace_back(*m_textureSampler,
                                                m_device->GetDummyTexture()->GetTexture().GetImageView(),
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
                }

                descSetWrites.emplace_back(m_materialTextureDescriptorSets[i], 1, 0, 1,
                                           vk::DescriptorType::eCombinedImageSampler, &descImageInfos.back());
            }
        }

        m_worldMatricesUBO.FillDescriptorSetWrite(descSetWrites.emplace_back());

        m_device->GetDevice().updateDescriptorSets(descSetWrites, nullptr);
    }

    void Mesh::UploadMeshData(QueuedDeviceTransfer& transfer)
    {
        assert(m_memoryGroup);
        m_memoryGroup->FinalizeDeviceGroup();
        m_memoryGroup->TransferData(transfer);
        m_vertexMaterialData.clear();
    }

    void Mesh::TransferWorldMatrices(vk::CommandBuffer transferCmdBuffer, std::size_t backbufferIdx) const
    {
        for (const auto& node : m_meshInfo->GetNodes()) {
            auto i = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
            m_worldMatricesUBO.FillUploadCmdBuffer<WorldMatrixUBO>(transferCmdBuffer, i);
        }
    }

    void Mesh::UpdateWorldMatrices(std::size_t backbufferIndex, const glm::mat4& worldMatrix) const
    {
        UpdateWorldMatricesNode(backbufferIndex, m_meshInfo->GetRootNode(), worldMatrix);
    }

    void Mesh::UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node, const glm::mat4& worldMatrix) const
    {
        if (!node->HasMeshes()) { return; }

        auto nodeWorld = worldMatrix * node->GetLocalTransform();

        WorldMatrixUBO worldMatrices{glm::mat4{1.0f}, glm::mat4{1.0f}};
        worldMatrices.m_model = nodeWorld;
        worldMatrices.m_normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(nodeWorld)));

        auto instanceIndex = backbufferIndex * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        m_worldMatricesUBO.UpdateInstanceData(instanceIndex, worldMatrices);

        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            UpdateWorldMatricesNode(backbufferIndex, node->GetChild(i), nodeWorld);
        }
    }

    void Mesh::Draw(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout) const
    {
        cmdBuffer.bindVertexBuffers(0, 1, m_vertexBuffer.first->GetBufferPtr(), &m_vertexBuffer.second);
        cmdBuffer.bindIndexBuffer(m_indexBuffer.first->GetBuffer(), m_indexBuffer.second, vk::IndexType::eUint32);

        DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, m_meshInfo->GetRootNode());
    }

    void Mesh::DrawNode(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout,
        const SceneMeshNode* node) const
    {
        if (!node->HasMeshes()) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        m_worldMatricesUBO.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, instanceIndex);

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            DrawSubMesh(cmdBuffer, pipelineLayout, m_meshInfo->GetSubMeshes()[node->GetSubMeshID(i)]);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, node->GetChild(i));
        }
    }

    void Mesh::DrawSubMesh(vk::CommandBuffer cmdBuffer, vk::PipelineLayout pipelineLayout, const SubMesh& subMesh) const
    {
        // bind material.
        auto& matDescSets = m_materialTextureDescriptorSets[subMesh.GetMaterialID()];
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, matDescSets, nullptr);
        m_materialsUBO.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, subMesh.GetMaterialID());

        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1, static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0);
    }

    void Mesh::GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx, RenderList& renderList) const
    {
        renderList.SetCurrentGeometry(m_vertexBuffer, m_indexBuffer);

        GetDrawElementsNode(worldMatrix, camera, backbufferIdx, m_meshInfo->GetRootNode(), renderList);
    }

    void Mesh::GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera,
        std::size_t backbufferIdx, const SceneMeshNode* node, RenderList& renderList) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;

        auto aabb = node->GetBoundingBox().NewFromTransform(nodeWorld);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        renderList.SetCurrentWorldMatrices(RenderElement::UBOBinding{ &m_worldMatricesUBO, 0, instanceIndex });

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            GetDrawElementsSubMesh(nodeWorld, camera, m_meshInfo->GetSubMeshes()[node->GetSubMeshID(i)], renderList);
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
        const auto mat = m_meshInfo->GetMaterial(subMesh.GetMaterialID());
        auto hasTransparency = (mat->m_alpha < 1.0f) || mat->m_hasAlpha;

        auto& matDescSets = m_materialTextureDescriptorSets[subMesh.GetMaterialID()];
        RenderElement::DescSetBinding materialTextureBinding(matDescSets, 2);
        RenderElement::UBOBinding materialBinding(&m_materialsUBO, 1, subMesh.GetMaterialID());

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
        m_vertexBuffer.first = vtxBuffer;
        m_vertexBuffer.second = offset;
    }

    void Mesh::SetIndexBuffer(const DeviceBuffer* idxBuffer, std::size_t offset)
    {
        m_indexBuffer.first = idxBuffer;
        m_indexBuffer.second = offset;
    }

}
