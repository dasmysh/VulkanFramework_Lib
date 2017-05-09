/**
 * @file   ForwardMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Definition of a forward renderer for meshes.
 */

#include "ForwardMeshRenderer.h"

#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/GraphicsPipeline.h"
#include "gfx/vk/memory/MemoryGroup.h"
#include "app/ApplicationBase.h"
#include "core/components/RenderComponent.h"
#include "core/components/TransformComponent.h"
#include "core/aligned_vector.h"
#include "Renderable.h"
#include "MeshMaterialVertex.h"
#include "gfx/Material.h"
#include "gfx/Texture2D.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace vku::gfx {

    ForwardMeshRenderer::ForwardMeshRenderer(const gfx::LogicalDevice* device, const ApplicationBase* app,
        unsigned int numSwapchainImages, const std::vector<std::uint32_t>& queueFamilyIndices) :
        device_{ device },
        application_{ app },
        numSwapchainImages_{ numSwapchainImages },
        queueFamilyIndices_{ queueFamilyIndices }
    {
        {
            std::vector<vk::DescriptorSetLayoutBinding> perFrameLayoutBindings; perFrameLayoutBindings.reserve(2);
            perFrameLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex); // camera matrices
            perFrameLayoutBindings.emplace_back(1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment); // linear sampler

            std::vector<vk::DescriptorSetLayoutBinding> perMaterialLayoutBindings; perMaterialLayoutBindings.reserve(3);
            perMaterialLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment); // material data
            perMaterialLayoutBindings.emplace_back(1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment); // diffuse texture
            perMaterialLayoutBindings.emplace_back(2, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment); // bump texture

            std::vector<vk::DescriptorSetLayoutBinding> perNodeLayoutBindings; perNodeLayoutBindings.reserve(1);
            perNodeLayoutBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex); // model / normal matrices

            vk::DescriptorSetLayoutCreateInfo perFrameLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(perFrameLayoutBindings.size()), perFrameLayoutBindings.data() };
            vkDescSetFrameLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(perFrameLayoutCreateInfo);

            vk::DescriptorSetLayoutCreateInfo perMaterialLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(perMaterialLayoutBindings.size()), perMaterialLayoutBindings.data() };
            vkDescSetMaterialLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(perMaterialLayoutCreateInfo);

            vk::DescriptorSetLayoutCreateInfo perNodeLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(),
                static_cast<std::uint32_t>(perNodeLayoutBindings.size()), perNodeLayoutBindings.data() };
            vkDescSetNodeLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(perNodeLayoutCreateInfo);
        }

        {
            std::vector<vk::DescriptorSetLayout> descriptorSets{ vkDescSetFrameLayout_.get(), vkDescSetMaterialLayout_.get(), vkDescSetNodeLayout_.get() };
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ vk::PipelineLayoutCreateFlags(),
                static_cast<std::uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr };
            vkPipelineLayout_ = device_->GetDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
        }
        
        {
            vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
                vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat };
            vkLinearSampler_ = device_->GetDevice().createSamplerUnique(samplerCreateInfo);
        }
    }

    ForwardMeshRenderer::ForwardMeshRenderer(ForwardMeshRenderer&& rhs) :
        device_{ rhs.device_ },
        application_{ rhs.application_ },
        numSwapchainImages_{ rhs.numSwapchainImages_ },
        queueFamilyIndices_{ rhs.queueFamilyIndices_ },
        vkPipelineLayout_{ std::move(rhs.vkPipelineLayout_) },
        vkDescriptorPool_{ std::move(rhs.vkDescriptorPool_) },
        vkDescSetFrameLayout_{ std::move(rhs.vkDescSetFrameLayout_) },
        vkDescSetMaterialLayout_{ std::move(rhs.vkDescSetMaterialLayout_) },
        vkDescSetNodeLayout_{ std::move(rhs.vkDescSetNodeLayout_) },
        vkDescSetFrame_{ std::move(rhs.vkDescSetFrame_) },
        vkDescSetsMaterials_{ std::move(rhs.vkDescSetsMaterials_) },
        vkDescSetNode_{ std::move(rhs.vkDescSetNode_) },
        vkLinearSampler_{ std::move(rhs.vkLinearSampler_) }

    {
    }

    ForwardMeshRenderer::~ForwardMeshRenderer() = default;

    void ForwardMeshRenderer::RegisterSceneObjects(const Scene& sceneObjects)
    {
        // std::vector<std::uint32_t> registeredSceneObjects;

        struct PartDescriptor {
            std::size_t vboOffset_;
            std::size_t iboOffset_;
            std::uint32_t firstIndex_;
            std::uint32_t indexCount_;
        };

        struct RenderingQueueObject {
            std::size_t materialIndex_;
            std::size_t transformIndex_;
            std::size_t subMeshIndex_;
            std::size_t renderableIndex_;
        };

        struct CameraContent {
            glm::mat4 projMatrix_;
            glm::mat4 viewMatrix_;
            glm::vec3 cameraPosition_;
        };

        // vbo/ibo -> renderer
        // material -> renderer [map with material name as a key + buffer]
        // transform -> renderer
        // submesh -> renderable

        MemoryGroup memoryGroup{ device_, vk::MemoryPropertyFlags() };

        std::vector<Material> materials;
        std::unordered_map<std::string, std::size_t> materialIndices;

        auto localTransformAlignment = device_->CalculateUniformBufferAlignment(sizeof(LocalTransform));
        auto materialAlignment = device_->CalculateUniformBufferAlignment(sizeof(MeshMaterial));

        aligned_vector<LocalTransform> transformUBOs{ localTransformAlignment }; // TODO: differentiate between static and dynamic later. [4/17/2017 Sebastian Maisch]
        std::vector<PartDescriptor> renderableParts;
        std::vector<RenderingQueueObject> renderingQueue;

        std::vector<Renderable*> registeredRenderables;

        std::size_t numIndices = 0, numVertices = 0;

        for (auto soHandle : sceneObjects) {
            auto& sceneObject = application_->GetSceneObjectManager().FromHandle(soHandle);

            if (sceneObject.HasComponent<RenderComponent>()) {
                auto renderComponent = sceneObject.GetComponent<RenderComponent>();
                auto renderable = renderComponent->GetRenderable();
                auto renderableIndex = registeredRenderables.size();
                registeredRenderables.push_back(renderable);

                auto iboOffset = numIndices * sizeof(std::uint32_t);
                auto vboOffset = numVertices * sizeof(MeshVertex);
                numIndices += renderable->GetTotalElementCount();
                numVertices += renderable->GetTotalVertexCount();

                auto basePartIndex = renderableParts.size();

                auto numMaterials = renderable->GetNumberOfMaterials();
                for (std::size_t materialID = 0; materialID < numMaterials; ++materialID) {
                    auto& materialInfo = renderable->GetMaterial(materialID);
                    auto matIt = materialIndices.find(materialInfo.materialName_);
                    std::size_t matIndex = 0;
                    if (matIt == materialIndices.end()) {
                        auto materialIndex = materials.size();
                        materialIndices[materialInfo.materialName_] = materialIndex;
                        materials.emplace_back(&materialInfo, device_, memoryGroup, queueFamilyIndices_);
                    } else {
                        matIndex = matIt->second;
                    }
                    renderableParts.emplace_back(vboOffset, iboOffset, renderable->GetFirstElement(materialID), renderable->GetElementCount(materialID));
                }

                auto modelMatrix = glm::mat4();
                if (sceneObject.HasComponent<TransformComponent>()) {
                    modelMatrix = sceneObject.GetComponent<TransformComponent>()->Matrix();
                }

                auto transformIndex = renderable->FillLocalTransforms(transformUBOs, modelMatrix);
                auto numNodes = renderable->GetNumberOfNodes();
                for (std::size_t nodeID = 0; nodeID < numNodes; ++nodeID) {
                    auto numParts = renderable->GetNumberOfPartsInNode(nodeID);

                    for (std::size_t partID = 0; partID < numParts; ++partID) {
                        auto materialIndex = materialIndices[renderable->GetMaterial(nodeID, partID).materialName_];
                        auto objectPartID = renderable->GetObjectPartID(nodeID, partID);
                        renderingQueue.emplace_back(materialIndex, transformIndex++, basePartIndex + objectPartID, renderableIndex);
                    }
                }
            }
        }

        std::vector<std::uint32_t> indices;
        std::vector<MeshVertex> vertices;
        vertices.reserve(numVertices);
        indices.reserve(numIndices);

        GatherMeshInfo(sceneObjects, indices, vertices);

        aligned_vector<MeshMaterial> materialUBOContent{ materialAlignment };
        materialUBOContent.reserve(materials.size());
        for (const auto& material : materials) materialUBOContent.emplace_back(material);

        const auto transformUBOsSize = byteSizeOf(transformUBOs);
        const auto cameraUBOSize = device_->CalculateUniformBufferAlignment(sizeof(CameraContent));
        const auto materialUBOsSize = byteSizeOf(materialUBOContent);
        const auto indicesSize = byteSizeOf(indices);
        const auto verticesSize = byteSizeOf(vertices);
        const auto cameraOffset = 0;
        const auto transformOffset = cameraOffset + numSwapchainImages_ * cameraUBOSize;
        const auto materialOffset = transformOffset + numSwapchainImages_ * transformUBOsSize;

        const auto bufferSize = materialOffset + materialUBOsSize + indicesSize + verticesSize;

        CameraContent camera;

        auto bufferIndex = memoryGroup.AddBufferToGroup(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
            | vk::BufferUsageFlagBits::eUniformBuffer, bufferSize, queueFamilyIndices_);
        for (auto i = 0U; i < numSwapchainImages_; ++i) memoryGroup.AddDataToBufferInGroup(bufferIndex, i * cameraUBOSize, cameraUBOSize, &camera);
        auto currentOffset = numSwapchainImages_ * cameraUBOSize;
        for (auto i = 0U; i < numSwapchainImages_; ++i) memoryGroup.AddDataToBufferInGroup(bufferIndex, currentOffset + i * transformUBOsSize, transformUBOs);
        currentOffset += numSwapchainImages_ * transformUBOsSize;

        memoryGroup.AddDataToBufferInGroup(bufferIndex, currentOffset, materialUBOContent);
        currentOffset += materialUBOsSize;
        memoryGroup.AddDataToBufferInGroup(bufferIndex, currentOffset, indices);
        currentOffset += indicesSize;
        memoryGroup.AddDataToBufferInGroup(bufferIndex, currentOffset, vertices);




        {
            std::array<vk::DescriptorPoolSize, 2> descSetPoolSizes;
            descSetPoolSizes[0] = vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1 };
            descSetPoolSizes[1] = vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 2 + materials.size() }; // UBOs materials + (camera + transforms)
            descSetPoolSizes[2] = vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 2 * materials.size() }; // 2*materials
            vk::DescriptorPoolCreateInfo descSetPoolInfo{ vk::DescriptorPoolCreateFlags(), 2 * static_cast<std::uint32_t>(materials.size()) + 4,
                static_cast<std::uint32_t>(descSetPoolSizes.size()), descSetPoolSizes.data() };
            vkDescriptorPool_ = device_->GetDevice().createDescriptorPoolUnique(descSetPoolInfo);
        }

        {
            std::vector<vk::DescriptorSetLayout> frameLayouts(1, vkDescSetFrameLayout_.get());
            vk::DescriptorSetAllocateInfo descSetAllocInfoFrame{ vkDescriptorPool_.get(), static_cast<std::uint32_t>(frameLayouts.size()), frameLayouts.data() };
            vkDescSetFrame_ = std::move(device_->GetDevice().allocateDescriptorSetsUnique(descSetAllocInfoFrame)[0]);

            std::vector<vk::DescriptorSetLayout> materialLayouts(materials.size(), vkDescSetMaterialLayout_.get());
            vk::DescriptorSetAllocateInfo descSetAllocInfoMaterials{ vkDescriptorPool_.get(), static_cast<std::uint32_t>(materialLayouts.size()), materialLayouts.data() };
            vkDescSetsMaterials_ = device_->GetDevice().allocateDescriptorSetsUnique(descSetAllocInfoMaterials);

            std::vector<vk::DescriptorSetLayout> nodeLayouts(1, vkDescSetNodeLayout_.get());
            vk::DescriptorSetAllocateInfo descSetAllocInfoNode{ vkDescriptorPool_.get(), static_cast<std::uint32_t>(nodeLayouts.size()), nodeLayouts.data() };
            vkDescSetNode_ = std::move(device_->GetDevice().allocateDescriptorSetsUnique(descSetAllocInfoNode)[0]);
        }

        {
            std::vector<vk::WriteDescriptorSet> descSetWrites; descSetWrites.reserve(2 * materials.size() + 4);
            vk::DescriptorImageInfo descSamplerInfo{ vkLinearSampler_.get() };
            descSetWrites.emplace_back(vkDescSetFrame_, 0, 0, 1, vk::DescriptorType::eSampler, &descSamplerInfo);
            vk::DescriptorBufferInfo descCamBufferInfo{ memoryGroup.GetBuffer(bufferIndex)->GetBuffer(), cameraOffset, cameraUBOSize };
            descSetWrites.emplace_back(vkDescSetFrame_, 1, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &descCamBufferInfo);

            for (std::size_t i = 0; i < materials.size(); ++i) {
                vk::DescriptorBufferInfo descMaterialInfo{ memoryGroup.GetBuffer(bufferIndex)->GetBuffer(), materialOffset + i * materialAlignment, materialAlignment };
                descSetWrites.emplace_back(vkDescSetsMaterials_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descMaterialInfo);
                vk::DescriptorImageInfo descDiffuseTexture{ vk::Sampler(), materials[i].diffuseTexture_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
                descSetWrites.emplace_back(vkDescSetsMaterials_[i], 1, 0, 1, vk::DescriptorType::eSampledImage, &descDiffuseTexture);
                vk::DescriptorImageInfo descBumpTexture{ vk::Sampler(), materials[i].bumpMap_->GetTexture().GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
                descSetWrites.emplace_back(vkDescSetsMaterials_[i], 2, 0, 1, vk::DescriptorType::eSampledImage, &descBumpTexture);
            }

            vk::DescriptorBufferInfo descTransformInfo{ memoryGroup.GetBuffer(bufferIndex)->GetBuffer(), transformOffset, localTransformAlignment };
            descSetWrites.emplace_back(vkDescSetNode_, 0, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &descTransformInfo);

            device_->GetDevice().updateDescriptorSets(descSetWrites, nullptr);
        }
    }

    void ForwardMeshRenderer::GatherMeshInfo(const Scene& sceneObjects,
        std::vector<std::uint32_t>& indices, std::vector<MeshVertex>& vertices)
    {
        for (auto soHandle : sceneObjects) {
            auto& sceneObject = application_->GetSceneObjectManager().FromHandle(soHandle);

            if (sceneObject.HasComponent<RenderComponent>()) {
                auto renderComponent = sceneObject.GetComponent<RenderComponent>();
                auto renderable = renderComponent->GetRenderable();

                renderable->GetIndices(indices);
                auto meshInfo = renderable->GetMeshInfo();

                auto numMeshVertices = renderable->GetTotalVertexCount();
                for (std::size_t i = 0; i < numMeshVertices; ++i) vertices.emplace_back(meshInfo, i);
            }
        }
    }

    void ForwardMeshRenderer::UpdateUniformLocations()
    {
        uniform_locations_ = program_->GetUniformLocations(
        { "modelMatrix", "normalMatrix", "viewProjectionMatrix", "diffuseTexture",
            "bumpTexture" });
    }

    void ForwardMeshRenderer::Draw(const Framebuffer& framebuffer,
        const Camera& camera, const Scene& scene)
    {

        framebuffer.DrawToFBO([&camera, &scene, this] {

            glUseProgram(program_->GetProgramId());

            glm::mat4 vp_matrix = camera.getProjMatrix() * camera.getViewMatrix();
            glUniformMatrix4fv(uniform_locations_[2], 1, GL_FALSE,
                reinterpret_cast<GLfloat*> (&vp_matrix));

            for (const auto game_object_handle : scene) {

                const auto& game_object =
                    engine_->game_object_manager_->FromHandle(game_object_handle);

                if (game_object.HasComponent<RenderComponent>()) {
                    auto render_component = game_object.GetComponent<RenderComponent>();
                    auto renderable = render_component->GetRenderable();

                    glm::mat4 model_matrix{ 1.0 };
                    if (game_object.HasComponent<TransformComponent>()) {
                        model_matrix =
                            game_object.GetComponent<TransformComponent>()->Matrix();
                    }

                    renderable->Bind(typeid(SimpleMeshVertex));

                    std::size_t number_of_nodes = renderable->GetNumberOfNodes();

                    for (std::size_t node_id = 0;
                        node_id < number_of_nodes; ++node_id) {
                        DrawNode(*renderable, model_matrix, node_id);
                    }

                    glBindVertexArray(0);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
            }
        });
    }

    /*void ForwardRenderer::DrawNode(const Renderable& render_component,
        const glm::mat4& model_matrix,
        std::size_t node_id)
    {
        const auto local_matrix =
            render_component.GetLocalTransform(node_id) * model_matrix;

        std::size_t number_of_parts_in_node = render_component.GetNumberOfPartsInNode(node_id);

        for (auto part_id = 0U;
            part_id < number_of_parts_in_node;
            ++part_id) {
            DrawNodePart(render_component, local_matrix, node_id, part_id);
        }
    }

    void ForwardRenderer::DrawNodePart(const Renderable& render_component,
        const glm::mat4& model_matrix,
        std::size_t node_id, std::size_t part_id)
    {

        glUniformMatrix4fv(uniform_locations_[0], 1, GL_FALSE,
            glm::value_ptr(model_matrix));
        glUniformMatrix3fv(
            uniform_locations_[1], 1, GL_FALSE,
            glm::value_ptr(glm::inverseTranspose(glm::mat3(model_matrix))));

        auto material = render_component.GetMaterial(node_id, part_id);

        if (material.diffuse_texture && uniform_locations_.size() > 3) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material.diffuse_texture->GetTextureId());
            glUniform1i(uniform_locations_[3], 0);
        }

        if (material.bump_texture && uniform_locations_.size() > 4) {
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, material.bump_texture->GetTextureId());
            glUniform1i(uniform_locations_[4], 1);
        }

        GLsizei element_count = render_component.GetElementCount(node_id, part_id);

        glDrawElements(GL_TRIANGLES,
            element_count,
            GL_UNSIGNED_INT,
            (static_cast<char*>(nullptr))
            + (render_component.GetElementOffset(node_id, part_id)
                * sizeof(unsigned int)));
    }*/
}
