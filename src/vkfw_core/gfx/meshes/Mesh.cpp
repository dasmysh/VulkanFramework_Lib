/**
 * @file   Mesh.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.24
 *
 * @brief  Implementation of the Mesh class.
 */

#include "gfx/meshes/Mesh.h"
#include "core/math/math.h"
#include "gfx/Texture2D.h"
#include "gfx/camera/CameraBase.h"
#include "gfx/renderer/RenderList.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkfw_core::gfx {

    Mesh::Mesh(const LogicalDevice* device, std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
               UniformBufferObject&& materialsUBO, std::size_t numBackbuffers,
               const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
        : m_device{device}
        , m_meshInfo{meshInfo}
        , m_internalMemoryGroup{std::make_unique<MemoryGroup>(device, fmt::format("MeshMemGroup:{}", name), memoryFlags)}
        , m_memoryGroup{m_internalMemoryGroup.get()}
        , m_bufferIdx{DeviceMemoryGroup::INVALID_INDEX}
        , m_vertexBuffer{nullptr, 0}
        , m_indexBuffer{nullptr, 0}
        , m_worldMatricesUBO{vkfw_core::gfx::UniformBufferObject::Create<WorldMatrixUBO>(
              device, numBackbuffers * meshInfo->GetNodes().size())}
        , m_materialsUBO{std::move(materialsUBO)}
        , m_worldMatricesDescriptorSetLayout{fmt::format("{} WorldMatricesDescSetLayout", name)}
        , m_materialDescriptorSetLayout{fmt::format("{} MaterialDescSetLayout", name)}
    {
        CreateMaterials(queueFamilyIndices);
        m_worldMatricesUBO.AddDescriptorLayoutBinding(m_worldMatricesDescriptorSetLayout,
                                                      vk::ShaderStageFlagBits::eVertex, true, 0);
    }

    Mesh::Mesh(const LogicalDevice* device, std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
               UniformBufferObject&& materialsUBO, std::size_t numBackbuffers, MemoryGroup& memoryGroup,
               unsigned int bufferIndex, const std::vector<std::uint32_t>& queueFamilyIndices)
        : m_device{device}
        , m_meshInfo{meshInfo}
        , m_memoryGroup{&memoryGroup}
        , m_bufferIdx{bufferIndex}
        , m_vertexBuffer{nullptr, 0}
        , m_indexBuffer{nullptr, 0}
        , m_worldMatricesUBO{vkfw_core::gfx::UniformBufferObject::Create<WorldMatrixUBO>(
              device, numBackbuffers * meshInfo->GetNodes().size())}
        , m_materialsUBO{std::move(materialsUBO)}
        , m_worldMatricesDescriptorSetLayout{fmt::format("{} WorldMatricesDescSetLayout", name)}
        , m_materialDescriptorSetLayout{fmt::format("{} MaterialDescSetLayout", name)}
    {
        CreateMaterials(queueFamilyIndices);
        m_worldMatricesUBO.AddDescriptorLayoutBinding(m_worldMatricesDescriptorSetLayout,
                                                      vk::ShaderStageFlagBits::eVertex, true, 0);
    }

    Mesh::Mesh(Mesh&& rhs) noexcept = default;

    Mesh& Mesh::operator=(Mesh&& rhs) noexcept = default;

    Mesh::~Mesh() = default;

    void Mesh::CreateMaterials(const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        // sets:
        // 0: world
        // 1: material UBO + textures
        // 2: camera (not handled in this class)
        {
            vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),
                                                    vk::Filter::eLinear,
                                                    vk::Filter::eLinear,
                                                    vk::SamplerMipmapMode::eNearest,
                                                    vk::SamplerAddressMode::eRepeat,
                                                    vk::SamplerAddressMode::eRepeat,
                                                    vk::SamplerAddressMode::eRepeat};
            m_textureSampler = m_device->GetHandle().createSamplerUnique(samplerCreateInfo);
        }

        {
            // Binding 0: Diffuse map
            Texture::AddDescriptorLayoutBinding(m_materialDescriptorSetLayout,
                                                vk::DescriptorType::eCombinedImageSampler,
                                                vk::ShaderStageFlagBits::eFragment, 0);
            // Binding 1: Bump map
            Texture::AddDescriptorLayoutBinding(m_materialDescriptorSetLayout,
                                                vk::DescriptorType::eCombinedImageSampler,
                                                vk::ShaderStageFlagBits::eFragment, 1);
            // Binding 2: Material UBO
            UniformBufferObject::AddDescriptorLayoutBinding(m_materialDescriptorSetLayout,
                                                            vk::ShaderStageFlagBits::eFragment, false, 2);
            m_materialDescriptorSetLayout.CreateDescriptorLayout(m_device);
        }

        m_materials.reserve(m_meshInfo->GetMaterials().size());
        for (const auto& mat : m_meshInfo->GetMaterials()) {
            m_materials.emplace_back(&mat, m_device, *m_memoryGroup, queueFamilyIndices);
        }
    }

    void Mesh::AddDescriptorPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, std::size_t& setCount) const
    {
        auto numMaterials = m_meshInfo->GetMaterials().size();

        m_worldMatricesDescriptorSetLayout.AddDescriptorPoolSizes(poolSizes, 1);
        m_materialDescriptorSetLayout.AddDescriptorPoolSizes(poolSizes, numMaterials);
        setCount = numMaterials + 1;
    }

    void Mesh::CreateDescriptorSets(vk::DescriptorPool descriptorPool)
    {
        m_materialDescriptorSetLayout.CreateDescriptorLayout(m_device);
        m_worldMatricesDescriptorSetLayout.CreateDescriptorLayout(m_device);

        auto numMaterials = m_materials.size();

        std::vector<vk::WriteDescriptorSet> descSetWrites;
        descSetWrites.resize(3 * numMaterials + 1);
        std::vector<vk::DescriptorBufferInfo> descBufferInfos;
        descBufferInfos.resize(numMaterials + 1);
        std::vector<vk::DescriptorImageInfo> descImageInfos;
        descImageInfos.resize(2 * numMaterials);

        {
            std::vector<vk::DescriptorSetLayout> materialDescSetLayouts;
            materialDescSetLayouts.resize(numMaterials, m_materialDescriptorSetLayout.GetHandle());
            vk::DescriptorSetAllocateInfo materialDescSetAllocInfo{
                descriptorPool, static_cast<std::uint32_t>(materialDescSetLayouts.size()),
                materialDescSetLayouts.data()};
            m_materialDescriptorSets = m_device->GetHandle().allocateDescriptorSets(materialDescSetAllocInfo);

            for (std::size_t i = 0; i < m_materials.size(); ++i) {
                if (m_materials[i].m_diffuseTexture) {
                    descImageInfos[2 * i] = vk::DescriptorImageInfo{
                        *m_textureSampler, m_materials[i].m_diffuseTexture->GetTexture().GetImageView().GetHandle(),
                        vk::ImageLayout::eShaderReadOnlyOptimal};
                } else {
                    descImageInfos[2 * i] = vk::DescriptorImageInfo{
                        *m_textureSampler, m_device->GetDummyTexture()->GetTexture().GetImageView().GetHandle(),
                        vk::ImageLayout::eShaderReadOnlyOptimal};
                }
                descSetWrites[3 * i] =
                    m_materialDescriptorSetLayout.MakeWrite(m_materialDescriptorSets[i], 0, &descImageInfos[2 * i]);

                if (m_materials[i].m_bumpMap) {
                    descImageInfos[2 * i + 1] = vk::DescriptorImageInfo{
                        *m_textureSampler, m_materials[i].m_bumpMap->GetTexture().GetImageView().GetHandle(),
                        vk::ImageLayout::eShaderReadOnlyOptimal};
                } else {
                    descImageInfos[2 * i + 1] = vk::DescriptorImageInfo{
                        *m_textureSampler, m_device->GetDummyTexture()->GetTexture().GetImageView().GetHandle(),
                        vk::ImageLayout::eShaderReadOnlyOptimal};
                }
                descSetWrites[3 * i + 1] =
                    m_materialDescriptorSetLayout.MakeWrite(m_materialDescriptorSets[i], 1, &descImageInfos[2 * i + 1]);

                m_materialsUBO.FillDescriptorBufferInfo(descBufferInfos[i]);
                descSetWrites[3 * i + 2] =
                    m_materialDescriptorSetLayout.MakeWrite(m_materialDescriptorSets[i], 2, &descBufferInfos[i]);
            }
        }

        {
            auto worldMatrixLayout = m_worldMatricesDescriptorSetLayout.GetHandle();
            vk::DescriptorSetAllocateInfo worldMatrixDescSetAllocInfo{descriptorPool, 1, &worldMatrixLayout};
            m_worldMatrixDescriptorSet = m_device->GetHandle().allocateDescriptorSets(worldMatrixDescSetAllocInfo)[0];

            m_worldMatricesUBO.FillDescriptorBufferInfo(descBufferInfos[numMaterials]);
            descSetWrites[3 * numMaterials] = m_worldMatricesDescriptorSetLayout.MakeWrite(
                m_worldMatrixDescriptorSet, 0, &descBufferInfos[numMaterials]);
        }

        m_device->GetHandle().updateDescriptorSets(descSetWrites, nullptr);
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

    void Mesh::UpdateWorldMatricesNode(std::size_t backbufferIndex, const SceneMeshNode* node,
                                       const glm::mat4& worldMatrix) const
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
        cmdBuffer.bindVertexBuffers(0, 1, m_vertexBuffer.first->GetHandlePtr(), &m_vertexBuffer.second);
        cmdBuffer.bindIndexBuffer(m_indexBuffer.first->GetHandle(), m_indexBuffer.second, vk::IndexType::eUint32);

        DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, m_meshInfo->GetRootNode());
    }

    void Mesh::DrawNode(vk::CommandBuffer cmdBuffer, std::size_t backbufferIdx, vk::PipelineLayout pipelineLayout,
                        const SceneMeshNode* node) const
    {
        if (!node->HasMeshes()) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, m_worldMatrixDescriptorSet,
                                     static_cast<std::uint32_t>(instanceIndex * m_worldMatricesUBO.GetInstanceSize()));

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
        auto& matDescSet = m_materialDescriptorSets[subMesh.GetMaterialID()];
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, matDescSet, nullptr);

        cmdBuffer.drawIndexed(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1,
                              static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0);
    }

    void Mesh::GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
                               RenderList& renderList) const
    {
        renderList.SetCurrentGeometry(m_vertexBuffer, m_indexBuffer);

        GetDrawElementsNode(worldMatrix, camera, backbufferIdx, m_meshInfo->GetRootNode(), renderList);
    }

    void Mesh::GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
                                   const SceneMeshNode* node, RenderList& renderList) const
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;

        auto aabb = node->GetBoundingBox().NewFromTransform(nodeWorld);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        renderList.SetCurrentWorldMatrices(RenderElement::UBOBinding{
            m_worldMatrixDescriptorSet, 0,
            static_cast<std::uint32_t>(instanceIndex * m_worldMatricesUBO.GetInstanceSize())});

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            GetDrawElementsSubMesh(nodeWorld, camera, m_meshInfo->GetSubMeshes()[node->GetSubMeshID(i)], renderList);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            GetDrawElementsNode(nodeWorld, camera, backbufferIdx, node->GetChild(i), renderList);
        }
    }

    void Mesh::GetDrawElementsSubMesh(const glm::mat4& worldMatrix, const CameraBase& camera, const SubMesh& subMesh,
                                      RenderList& renderList) const
    {
        auto aabb = subMesh.GetLocalAABB().NewFromTransform(worldMatrix);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind material.
        const auto mat = m_meshInfo->GetMaterial(subMesh.GetMaterialID());
        auto hasTransparency = (mat->m_alpha < 1.0f) || mat->m_hasAlpha;

        auto& matDescSet = m_materialDescriptorSets[subMesh.GetMaterialID()];
        RenderElement::DescSetBinding materialBinding(matDescSet, 1);

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

        re->BindDescriptorSet(materialBinding);
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
