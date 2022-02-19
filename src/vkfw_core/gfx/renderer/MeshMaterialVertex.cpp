/**
 * @file   MeshVertex.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Implementations for the vertex format for meshes.
 */

#include "gfx/renderer/MeshMaterialVertex.h"
#include "gfx/meshes/MeshInfo.h"

namespace vkfw_core::gfx {

    vk::VertexInputBindingDescription MeshVertex::m_bindingDescription{ 0, sizeof(MeshVertex), vk::VertexInputRate::eVertex };
    std::array<vk::VertexInputAttributeDescription, 4> MeshVertex::m_attributeDescriptions{ {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, m_position) }, // NOLINT
         {1, 0, vk::Format::eR32G32Sfloat, offsetof(MeshVertex, m_texCoord)},    // NOLINT
         {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, m_normal)},   // NOLINT
         {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, m_tangent)}}}; // NOLINT

    MeshVertex::MeshVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index) :
        m_position{ mi->GetVertices()[index] },
        m_texCoord{ mi->GetTexCoords()[0][index] },
        m_normal { mi->GetNormals()[index] },
        m_tangent{ mi->GetTangents()[index] }
    {
    }
}

