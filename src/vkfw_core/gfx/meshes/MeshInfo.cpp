/**
 * @file   MeshInfo.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.13
 *
 * @brief  Contains the implementation of the MeshInfo class.
 */

#include "gfx/meshes/MeshInfo.h"
#include "gfx/meshes/SubMesh.h"
#include "gfx/Material.h"
#include "core/serialization_helper.h"
#include "gfx/meshes/SceneMeshNode.h"
#include <gfx/vk/buffers/DeviceBuffer.h>
#include <fstream>


namespace vku::gfx {

    /** Default constructor. */
    MeshInfo::MeshInfo() {}

    /** Copy constructor. */
    MeshInfo::MeshInfo(const MeshInfo& rhs) :
        vertices_(rhs.vertices_),
        normals_(rhs.normals_),
        texCoords_(rhs.texCoords_),
        tangents_(rhs.tangents_),
        binormals_(rhs.binormals_),
        colors_(rhs.colors_),
        boneOffsetMatrixIndices_(rhs.boneOffsetMatrixIndices_),
        boneWeights_(rhs.boneWeights_),
        indexVectors_(rhs.indexVectors_),
        inverseBindPoseMatrices_(rhs.inverseBindPoseMatrices_),
        boneParent_(rhs.boneParent_),
        indices_(rhs.indices_),
        materials_(rhs.materials_),
        animations_(rhs.animations_),
        rootNode_(std::make_unique<SceneMeshNode>(*rhs.rootNode_)),
        globalInverse_(rhs.globalInverse_),
        boneBoundingBoxes_(rhs.boneBoundingBoxes_)
    {
        for (const auto& submesh : rhs.subMeshes_) {
            subMeshes_.emplace_back(submesh);
        }
        rootNode_->FlattenNodeTree(nodes_);
    }

    /** Copy assignment operator. */
    MeshInfo& MeshInfo::operator=(const MeshInfo& rhs)
    {
        if (this != &rhs) {
            MeshInfo tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    /** Default move constructor. */
    MeshInfo::MeshInfo(MeshInfo&& rhs) noexcept :
        vertices_(std::move(rhs.vertices_)),
        normals_(std::move(rhs.normals_)),
        texCoords_(std::move(rhs.texCoords_)),
        tangents_(std::move(rhs.tangents_)),
        binormals_(std::move(rhs.binormals_)),
        colors_(std::move(rhs.colors_)),
        boneOffsetMatrixIndices_(std::move(boneOffsetMatrixIndices_)),
        boneWeights_(std::move(boneWeights_)),
        indexVectors_(std::move(indexVectors_)),
        inverseBindPoseMatrices_(std::move(inverseBindPoseMatrices_)),
        boneParent_(std::move(boneParent_)),
        indices_(std::move(rhs.indices_)),
        materials_(std::move(materials_)),
        subMeshes_(std::move(subMeshes_)),
        nodes_(std::move(rhs.nodes_)),
        animations_(std::move(animations_)),
        rootNode_(std::move(rhs.rootNode_)),
        globalInverse_(std::move(rhs.globalInverse_)),
        boneBoundingBoxes_(std::move(rhs.boneBoundingBoxes_))
    {
    }

    /** Default move assignment operator. */
    MeshInfo& MeshInfo::operator=(MeshInfo&& rhs) noexcept
    {
        this->~MeshInfo();
        vertices_ = std::move(rhs.vertices_);
        normals_ = std::move(rhs.normals_);
        texCoords_ = std::move(rhs.texCoords_);
        tangents_ = std::move(rhs.tangents_);
        binormals_ = std::move(rhs.binormals_);
        colors_ = std::move(rhs.colors_);
        boneOffsetMatrixIndices_ = std::move(rhs.boneOffsetMatrixIndices_);
        boneWeights_ = std::move(rhs.boneWeights_);
        indexVectors_ = std::move(rhs.indexVectors_);
        inverseBindPoseMatrices_ = std::move(rhs.inverseBindPoseMatrices_);
        boneParent_ = std::move(rhs.boneParent_);
        indices_ = std::move(rhs.indices_);
        materials_ = std::move(rhs.materials_);
        subMeshes_ = std::move(rhs.subMeshes_);
        nodes_ = std::move(rhs.nodes_);
        animations_ = std::move(rhs.animations_);
        rootNode_ = std::move(rhs.rootNode_);
        globalInverse_ = std::move(rhs.globalInverse_);
        boneBoundingBoxes_ = std::move(rhs.boneBoundingBoxes_);
        return *this;
    }

    /** Default destructor. */
    MeshInfo::~MeshInfo() = default;

    /**
     *  Reserves memory to create the mesh.
     *  @param maxUVChannels the maximum number of texture coordinates in a single sub-mesh vertex.
     *  @param maxColorChannels the maximum number of colors in a single sub-mesh vertex.
     *  @param numVertices the number of vertices in the mesh.
     *  @param numIndices the number of indices in the mesh.
     */
    void MeshInfo::ReserveMesh(unsigned int maxUVChannels, unsigned int maxColorChannels, bool hasTangentSpace,
        unsigned int numVertices, unsigned int numIndices, unsigned int numMaterials)
    {
        vertices_.resize(numVertices);
        normals_.resize(numVertices);
        texCoords_.resize(maxUVChannels);
        for (auto& texCoords : texCoords_) texCoords.resize(numVertices);
        if (hasTangentSpace) {
            tangents_.resize(numVertices);
            binormals_.resize(numVertices);
        }
        colors_.resize(maxColorChannels);
        for (auto& colors : colors_) colors.resize(numVertices);
        indices_.resize(numIndices);
        materials_.resize(numMaterials);
    }

    void MeshInfo::AddSubMesh(const std::string& name, unsigned int idxOffset, unsigned int numIndices, unsigned int materialID)
    {
        subMeshes_.emplace_back(this, name, idxOffset, numIndices, materialID);
    }

    void MeshInfo::CreateSceneNodes(aiNode* rootNode, const std::map<std::string, unsigned int>& boneMap)
    {
        rootNode_ = std::make_unique<SceneMeshNode>(rootNode, nullptr, boneMap);
        rootNode_->GenerateBoundingBoxes(*this);
        GenerateBoneBoundingBoxes();
        globalInverse_ = glm::inverse(rootNode_->GetLocalTransform());
    }

    ///
    /// Generate all BoundingBoxes for the bones.
    ///
    void MeshInfo::GenerateBoneBoundingBoxes()
    {
        if (inverseBindPoseMatrices_.empty()) {
            return;
        }

        boneBoundingBoxes_.resize(inverseBindPoseMatrices_.size());

        bool hasVertexWithoutBone = false;

        for (auto i = 0U; i < boneOffsetMatrixIndices_.size(); i++) {

            bool vertexHasBone = false;

            for (auto b = 0; b < 4; b++) {
                auto boneI = boneOffsetMatrixIndices_[i][b];
                auto boneW = boneWeights_[i][b];
                if (boneW > 0) {
                    vertexHasBone = true;
                    math::AABB3<float>& box = boneBoundingBoxes_[boneI];
                    box.AddPoint(vertices_[i]);
                }
            }
            if (!vertexHasBone) {
                hasVertexWithoutBone = true;
            }
        }

        if (hasVertexWithoutBone) {
            spdlog::warn("You are using a model where not all vertices in the "
                "model are associated with a bone! This can lead to "
                "errors in the collision detection!");
        }
    }

    void MeshInfo::FlattenHierarchies()
    {
        rootNode_->FlattenNodeTree(nodes_);
        std::map<std::string, std::size_t> nodeIndexMap;
        for (const auto& node : nodes_) { nodeIndexMap[node->GetName()] = node->GetNodeIndex(); }

        for (auto& animation : animations_) animation.FlattenHierarchy(nodes_.size(), nodeIndexMap);
    }
}
