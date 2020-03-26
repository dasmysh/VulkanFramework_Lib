/**
 * @file   SubMesh.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Implementation of the sub mesh class.
 */

#include "gfx/meshes/SubMesh.h"
#include "gfx/meshes/MeshInfo.h"

namespace vku::gfx {

    /** Constructor. */
    SubMesh::SubMesh(const MeshInfo* mesh, const std::string& objectName, unsigned int indexOffset, unsigned int numIndices, unsigned int materialID) :
        objectName_(objectName),
        indexOffset_(indexOffset),
        numIndices_(numIndices),
        materialID_(materialID)
    {
        if (numIndices_ == 0) return;
        auto& vertices = mesh->GetVertices();
        auto& indices = mesh->GetIndices();
        aabb_.minmax_[0] = aabb_.minmax_[1] = vertices[indices[indexOffset_]];
        for (auto i = indexOffset_; i < indexOffset_ + numIndices_; ++i) {
            aabb_.minmax_[0] = glm::min(aabb_.minmax_[0], vertices[indices[i]]);
            aabb_.minmax_[1] = glm::max(aabb_.minmax_[1], vertices[indices[i]]);
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
        objectName_(std::move(rhs.objectName_)),
        indexOffset_(std::move(rhs.indexOffset_)),
        numIndices_(std::move(rhs.numIndices_)),
        aabb_(std::move(rhs.aabb_)),
        materialID_(std::move(rhs.materialID_))
    {
    }

    /** Default move assignment operator. */
    SubMesh& SubMesh::operator=(SubMesh&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~SubMesh();
            objectName_ = std::move(rhs.objectName_);
            indexOffset_ = std::move(rhs.indexOffset_);
            numIndices_ = std::move(rhs.numIndices_);
            aabb_ = std::move(rhs.aabb_);
            materialID_ = std::move(rhs.materialID_);
        }
        return *this;
    }
}
