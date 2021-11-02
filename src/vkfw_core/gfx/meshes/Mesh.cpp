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
#include "gfx/vk/wrappers/VertexInputResources.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkfw_core::gfx {

    Mesh::Mesh(const LogicalDevice* device, std::string_view name, const std::shared_ptr<const MeshInfo>& meshInfo,
               UniformBufferObject&& materialsUBO, std::size_t numBackbuffers,
               const vk::MemoryPropertyFlags& memoryFlags, const std::vector<std::uint32_t>& queueFamilyIndices)
        : m_device{device}
        , m_name{name}
        , m_meshInfo{meshInfo}
        , m_internalMemoryGroup{std::make_unique<MemoryGroup>(device, fmt::format("MeshMemGroup:{}", name), memoryFlags)}
        , m_memoryGroup{m_internalMemoryGroup.get()}
        , m_bufferIdx{DeviceMemoryGroup::INVALID_INDEX}
        , m_worldMatricesUBO{vkfw_core::gfx::UniformBufferObject::Create<mesh::WorldUniformBufferObject>(
              device, numBackbuffers * meshInfo->GetNodes().size())}
        , m_materialsUBO{std::move(materialsUBO)}
        , m_textureSampler{device->GetHandle(), fmt::format("MeshSampler:{}", name), vk::UniqueSampler{}}
        , m_worldMatricesDescriptorSetLayout{fmt::format("{} WorldMatricesDescSetLayout", name)}
        , m_worldMatrixDescriptorSet{device, fmt::format("WorldMatrixDescSet:{}", name), vk::DescriptorSet{}}
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
        , m_name{name}
        , m_meshInfo{meshInfo}
        , m_memoryGroup{&memoryGroup}
        , m_bufferIdx{bufferIndex}
        , m_worldMatricesUBO{vkfw_core::gfx::UniformBufferObject::Create<mesh::WorldUniformBufferObject>(
              device, numBackbuffers * meshInfo->GetNodes().size())}
        , m_materialsUBO{std::move(materialsUBO)}
        , m_textureSampler{device->GetHandle(), fmt::format("MeshSampler:{}", name), vk::UniqueSampler{}}
        , m_worldMatricesDescriptorSetLayout{fmt::format("{} WorldMatricesDescSetLayout", name)}
        , m_worldMatrixDescriptorSet{device, fmt::format("WorldMatrixDescSet:{}", name),
                                     vk::DescriptorSet{}}
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
            m_textureSampler.SetHandle(m_device->GetHandle(), m_device->GetHandle().createSamplerUnique(samplerCreateInfo));
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

    void Mesh::CreateDescriptorSets(const DescriptorPool& descriptorPool)
    {
        if (!m_materialDescriptorSetLayout) { m_materialDescriptorSetLayout.CreateDescriptorLayout(m_device); }
        m_worldMatricesDescriptorSetLayout.CreateDescriptorLayout(m_device);

        auto numMaterials = m_materials.size();

        {
            std::vector<vk::DescriptorSetLayout> materialDescSetLayouts;
            materialDescSetLayouts.resize(numMaterials, m_materialDescriptorSetLayout.GetHandle());
            vk::DescriptorSetAllocateInfo materialDescSetAllocInfo{
                descriptorPool.GetHandle(), static_cast<std::uint32_t>(materialDescSetLayouts.size()),
                materialDescSetLayouts.data()};
            m_materialDescriptorSets =
                DescriptorSet::Initialize(m_device, fmt::format("{}:MaterialDescriptorSet", m_name),
                                          m_device->GetHandle().allocateDescriptorSets(materialDescSetAllocInfo));

            for (std::size_t i = 0; i < m_materials.size(); ++i) {
                m_materialDescriptorSets[i].InitializeWrites(m_device, m_materialDescriptorSetLayout);

                std::array<Texture*, 1> diffuseTexture = {nullptr};
                if (m_materials[i].m_diffuseTexture) {
                    diffuseTexture[0] = &m_materials[i].m_diffuseTexture->GetTexture();
                } else {
                    diffuseTexture[0] = &m_device->GetDummyTexture()->GetTexture();
                }
                m_materialDescriptorSets[i].WriteImageDescriptor(0, 0, diffuseTexture, m_textureSampler,
                                                                 vk::AccessFlagBits2KHR::eShaderRead,
                                                                 vk::ImageLayout::eShaderReadOnlyOptimal);

                std::array<Texture*, 1> bumpMap = {nullptr};
                if (m_materials[i].m_bumpMap) {
                    bumpMap[0] = &m_materials[i].m_bumpMap->GetTexture();
                } else {
                    bumpMap[0] = &m_device->GetDummyTexture()->GetTexture();
                }
                m_materialDescriptorSets[i].WriteImageDescriptor(1, 0, bumpMap, m_textureSampler,
                                                                 vk::AccessFlagBits2KHR::eShaderRead,
                                                                 vk::ImageLayout::eShaderReadOnlyOptimal);

                std::array<BufferRange, 1> bufferRange;
                m_materialsUBO.FillBufferRange(bufferRange[0]);
                m_materialDescriptorSets[i].WriteBufferDescriptor(2, 0, bufferRange,
                                                                  vk::AccessFlagBits2KHR::eShaderRead);

                m_materialDescriptorSets[i].FinalizeWrite(m_device);
            }
        }

        {
            auto worldMatrixLayout = m_worldMatricesDescriptorSetLayout.GetHandle();
            vk::DescriptorSetAllocateInfo worldMatrixDescSetAllocInfo{descriptorPool.GetHandle(), 1, &worldMatrixLayout};
            m_worldMatrixDescriptorSet.SetHandle(
                m_device->GetHandle(), fmt::format("{}:WorldMatrixDescriptorSet", m_name),
                std::move(m_device->GetHandle().allocateDescriptorSets(worldMatrixDescSetAllocInfo)[0]));

            std::array<BufferRange, 1> bufferRange;
            m_worldMatricesUBO.FillBufferRange(bufferRange[0]);
            m_worldMatrixDescriptorSet.InitializeWrites(m_device, m_worldMatricesDescriptorSetLayout);
            m_worldMatrixDescriptorSet.WriteBufferDescriptor(0, 0, bufferRange, vk::AccessFlagBits2KHR::eShaderRead);
            m_worldMatrixDescriptorSet.FinalizeWrite(m_device);
        }
    }

    void Mesh::UploadMeshData(QueuedDeviceTransfer& transfer)
    {
        assert(m_memoryGroup);
        m_memoryGroup->FinalizeDeviceGroup();
        m_memoryGroup->TransferData(transfer);
        m_vertexMaterialData.clear();
    }

    void Mesh::TransferWorldMatrices(CommandBuffer& transferCmdBuffer, std::size_t backbufferIdx) const
    {
        for (const auto& node : m_meshInfo->GetNodes()) {
            auto i = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
            m_worldMatricesUBO.FillUploadCmdBuffer<mesh::WorldUniformBufferObject>(transferCmdBuffer, i);
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

        mesh::WorldUniformBufferObject worldMatrices{glm::mat4{1.0f}, glm::mat4{1.0f}};
        worldMatrices.model = nodeWorld;
        worldMatrices.normalMatrix = glm::mat4(glm::inverseTranspose(glm::mat3(nodeWorld)));

        auto instanceIndex = backbufferIndex * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        m_worldMatricesUBO.UpdateInstanceData(instanceIndex, worldMatrices);

        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            UpdateWorldMatricesNode(backbufferIndex, node->GetChild(i), nodeWorld);
        }
    }

    void Mesh::Draw(CommandBuffer& cmdBuffer, std::size_t backbufferIdx,
                    const PipelineLayout& pipelineLayout)
    {
        m_vertexInput->Bind(cmdBuffer);
        DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, m_meshInfo->GetRootNode());
    }

    void Mesh::DrawNode(CommandBuffer& cmdBuffer, std::size_t backbufferIdx, const PipelineLayout& pipelineLayout,
                        const SceneMeshNode* node)
    {
        if (!node->HasMeshes()) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        m_worldMatrixDescriptorSet.Bind(
            cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 0,
            static_cast<std::uint32_t>(instanceIndex * m_worldMatricesUBO.GetInstanceSize()));

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            DrawSubMesh(cmdBuffer, pipelineLayout, m_meshInfo->GetSubMeshes()[node->GetSubMeshID(i)]);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            DrawNode(cmdBuffer, backbufferIdx, pipelineLayout, node->GetChild(i));
        }
    }

    void Mesh::DrawSubMesh(CommandBuffer& cmdBuffer, const PipelineLayout& pipelineLayout,
                           const SubMesh& subMesh)
    {
        // bind material.
        auto& matDescSet = m_materialDescriptorSets[subMesh.GetMaterialID()];
        matDescSet.Bind(cmdBuffer, vk::PipelineBindPoint::eGraphics, pipelineLayout, 1);

        cmdBuffer.GetHandle().drawIndexed(static_cast<std::uint32_t>(subMesh.GetNumberOfIndices()), 1,
                              static_cast<std::uint32_t>(subMesh.GetIndexOffset()), 0, 0);
    }

    void Mesh::GetDrawElements(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
                               RenderList& renderList)
    {
        renderList.SetCurrentGeometry(m_vertexInput.get());

        GetDrawElementsNode(worldMatrix, camera, backbufferIdx, m_meshInfo->GetRootNode(), renderList);
    }

    void Mesh::GetDrawElementsNode(const glm::mat4& worldMatrix, const CameraBase& camera, std::size_t backbufferIdx,
                                   const SceneMeshNode* node, RenderList& renderList)
    {
        auto nodeWorld = node->GetLocalTransform() * worldMatrix;

        auto aabb = node->GetBoundingBox().NewFromTransform(nodeWorld);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind world matrices
        auto instanceIndex = backbufferIdx * m_meshInfo->GetNodes().size() + node->GetNodeIndex();
        renderList.SetCurrentWorldMatrices(RenderElement::UBOBinding{
            &m_worldMatrixDescriptorSet, 0,
            static_cast<std::uint32_t>(instanceIndex * m_worldMatricesUBO.GetInstanceSize())});

        for (unsigned int i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            GetDrawElementsSubMesh(nodeWorld, camera, m_meshInfo->GetSubMeshes()[node->GetSubMeshID(i)], renderList);
        }
        for (unsigned int i = 0; i < node->GetNumberOfNodes(); ++i) {
            GetDrawElementsNode(nodeWorld, camera, backbufferIdx, node->GetChild(i), renderList);
        }
    }

    void Mesh::GetDrawElementsSubMesh(const glm::mat4& worldMatrix, const CameraBase& camera, const SubMesh& subMesh,
                                      RenderList& renderList)
    {
        auto aabb = subMesh.GetLocalAABB().NewFromTransform(worldMatrix);
        if (!math::AABBInFrustumTest(camera.GetViewFrustum(), aabb)) { return; }

        // bind material.
        const auto mat = m_meshInfo->GetMaterial(subMesh.GetMaterialID());
        auto hasTransparency = (mat->m_alpha < 1.0f) || mat->m_hasAlpha;

        auto& matDescSet = m_materialDescriptorSets[subMesh.GetMaterialID()];
        RenderElement::DescSetBinding materialBinding{&matDescSet, 1};

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

    void Mesh::SetVertexInput(DeviceBuffer* vtxBuffer, std::size_t vtxOffset, DeviceBuffer* idxBuffer,
                              std::size_t idxOffset)
    {
        std::array<BufferDescription, 1> vertexBuffer{{vtxBuffer, vtxOffset}};
        BufferDescription indexBuffer{idxBuffer, idxOffset};
        m_vertexInput =
            std::make_unique<VertexInputResources>(m_device, 0, vertexBuffer, indexBuffer, vk::IndexType::eUint32);
    }

}
