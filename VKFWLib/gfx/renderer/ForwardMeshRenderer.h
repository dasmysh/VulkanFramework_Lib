/**
 * @file   ForwardMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Definition of a forward renderer for meshes.
 */

#pragma once

#include "main.h"
#include "MeshMaterialVertex.h"

namespace vku {
    class ApplicationBase;
}

namespace vku::gfx {

    class GraphicsPipeline;
    class LogicalDevice;
    class Framebuffer;
    class Camera;
    using Scene = std::vector<std::uint32_t>;

    class ForwardMeshRenderer
    {
    public:
        ForwardMeshRenderer(const gfx::LogicalDevice* device,
            const ApplicationBase* app, unsigned int numSwapchainImages,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ForwardMeshRenderer(const ForwardMeshRenderer&) = delete;
        ForwardMeshRenderer& operator=(const ForwardMeshRenderer&) = delete;
        ForwardMeshRenderer(ForwardMeshRenderer&&);
        ForwardMeshRenderer& operator=(ForwardMeshRenderer&&);
        ~ForwardMeshRenderer();

        void Draw(const Framebuffer&, const Camera&, const Scene&);
        void RegisterSceneObjects(const Scene& sceneObjects);

        void GatherMeshInfo(const Scene& sceneObjects, std::vector<std::uint32_t>& indices, std::vector<MeshVertex>& vertices);

    protected:

    private:
        /** Holds the device. */
        const gfx::LogicalDevice* device_;
        /** Holds the application. */
        const ApplicationBase* application_;
        /** Holds the number of swapchain images. */
        const unsigned int numSwapchainImages_;
        /** Holds the queue family indices used for buffers and textures. */
        std::vector<std::uint32_t> queueFamilyIndices_;
        /** Holds the pipeline layout for demo rendering. */
        vk::PipelineLayout vkPipelineLayout_;
        /** Holds the descriptor pool for the UBO binding. */
        vk::DescriptorPool vkUBODescriptorPool_;
        /** Holds the descriptor sets for the UBO binding. */
        std::vector<vk::DescriptorSet> vkUBODescritorSets_;
        /** Holds the descriptor sets for the sampler binding. */
        std::vector<vk::DescriptorSet> vkSamplerDescritorSets_;
        /** Holds the descriptor sets for the image binding. */
        std::vector<vk::DescriptorSet> vkImageDescritorSets_;
        /** Holds the graphics pipeline for demo rendering. */
        std::unique_ptr<vku::gfx::GraphicsPipeline> demoPipeline_;
    };

}
