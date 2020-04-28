/**
 * @file   SubMesh.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Implementation of the sub mesh class.
 */

#include "gfx/meshes/SubMesh.h"
#include "gfx/meshes/MeshInfo.h"

namespace vkfw_core::gfx {

    /** Constructor. */
    SubMesh::SubMesh(const MeshInfo* mesh, std::string objectName, unsigned int indexOffset, unsigned int numIndices, unsigned int materialID) :
        m_objectName(std::move(objectName)),
        m_indexOffset(indexOffset),
        m_numIndices(numIndices),
        m_materialID(materialID)
    {
        if (m_numIndices == 0) { return; }
        auto& vertices = mesh->GetVertices();
        auto& indices = mesh->GetIndices();
        m_aabb.m_minmax[0] = m_aabb.m_minmax[1] = vertices[indices[m_indexOffset]];
        for (auto i = m_indexOffset; i < m_indexOffset + m_numIndices; ++i) {
            m_aabb.m_minmax[0] = glm::min(m_aabb.m_minmax[0], vertices[indices[i]]);
            m_aabb.m_minmax[1] = glm::max(m_aabb.m_minmax[1], vertices[indices[i]]);
        }
    }

    /** Default destructor. */
    SubMesh::~SubMesh() = default;
    /** Default copy constructor. */
    SubMesh::SubMesh(const SubMesh&) = default;
    /** Default copy assignment operator. */
    SubMesh& SubMesh::operator=(const SubMesh&) = default;

    /** Default move constructor. */
    SubMesh::SubMesh(SubMesh&& rhs) noexcept :
        m_objectName(std::move(rhs.m_objectName)),
        m_indexOffset(rhs.m_indexOffset),
        m_numIndices(rhs.m_numIndices),
        m_aabb(rhs.m_aabb),
        m_materialID(rhs.m_materialID)
    {
    }

    /** Default move assignment operator. */
    SubMesh& SubMesh::operator=(SubMesh&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~SubMesh();
            m_objectName = std::move(rhs.m_objectName);
            m_indexOffset = rhs.m_indexOffset;
            m_numIndices = rhs.m_numIndices;
            m_aabb = rhs.m_aabb;
            m_materialID = rhs.m_materialID;
        }
        return *this;
    }
}
