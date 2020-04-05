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

    vk::VertexInputBindingDescription MeshVertex::bindingDescription_{ 0, sizeof(MeshVertex), vk::VertexInputRate::eVertex };
    std::array<vk::VertexInputAttributeDescription, 4> MeshVertex::attributeDescriptions_{ {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, position_) }, // NOLINT
         {1, 0, vk::Format::eR32G32Sfloat, offsetof(MeshVertex, texCoord_)},    // NOLINT
         {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, normal_)},   // NOLINT
         {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, tangent_)}}}; // NOLINT

    MeshVertex::MeshVertex(const vkfw_core::gfx::MeshInfo* mi, std::size_t index) :
        position_{ mi->GetVertices()[index] },
        texCoord_{ mi->GetTexCoords()[0][index] },
        normal_ { mi->GetNormals()[index] },
        tangent_{ mi->GetTangents()[index] }
    {
    }
}
