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

namespace vkfw_core::gfx {

    class MeshInfo;
    struct Material;

    struct MeshVertex
    {
        glm::vec3 m_position = glm::vec3{0.0};
        glm::vec2 m_texCoord = glm::vec2{0.0};
        glm::vec3 m_normal = glm::vec3{0.0};
        glm::vec3 m_tangent = glm::vec3{0.0};

        MeshVertex() = default;
        MeshVertex(const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent) :
            m_position{ position }, m_texCoord{ texCoord }, m_normal{ normal }, m_tangent{ tangent } {};
        MeshVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index);
        static vk::VertexInputBindingDescription m_bindingDescription;
        static std::array<vk::VertexInputAttributeDescription, 4> m_attributeDescriptions;
    };

    struct MeshMaterial
    {
        explicit MeshMaterial(const Material&) {}
    };
}

