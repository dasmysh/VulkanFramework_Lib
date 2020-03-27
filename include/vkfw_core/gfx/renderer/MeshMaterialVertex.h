/**
 * @file   MeshVertex.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Declaration of the vertex format for meshes.
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace vku::gfx {

    class MeshInfo;
    struct Material;

    struct MeshVertex
    {
        glm::vec3 position_ = glm::vec3{0.0};
        glm::vec2 texCoord_ = glm::vec2{0.0};
        glm::vec3 normal_ = glm::vec3{0.0};
        glm::vec3 tangent_ = glm::vec3{0.0};

        MeshVertex() = default;
        MeshVertex(const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent) :
            position_{ position }, texCoord_{ texCoord }, normal_{ normal }, tangent_{ tangent } {};
        MeshVertex(const vku::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription bindingDescription_;
        static std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions_;
    };

    struct MeshMaterial
    {
        explicit MeshMaterial(const Material&) {}
    };
}

