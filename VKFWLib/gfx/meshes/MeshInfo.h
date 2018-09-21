/**
 * @file   MeshInfo.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.13
 *
 * @brief  Contains the definition of the MeshInfo class.
 */

#pragma once

#include "main.h"
#include <typeindex>
#include "Animation.h"
#include "SubMesh.h"
#include "SceneMeshNode.h"
#include "gfx/Material.h"
#include <core/serialization_helper.h>
#include <cereal/cereal.hpp>

struct aiNode;

namespace vku::gfx {

    class DeviceBuffer;

    /**
     * @brief  Base class for all meshes.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2014.01.13
     */
    class MeshInfo
    {
    public:
        MeshInfo();
        MeshInfo(const MeshInfo&);
        MeshInfo& operator=(const MeshInfo&);
        MeshInfo(MeshInfo&&) noexcept;
        MeshInfo& operator=(MeshInfo&&) noexcept;
        virtual ~MeshInfo();

        unsigned int GetNumSubmeshes() const { return static_cast<unsigned int>(subMeshes_.size()); }
        const SubMesh* GetSubMesh(std::size_t id) const { return subMeshes_[id].get(); }

        const std::vector<glm::vec3>& GetVertices() const { return vertices_; }
        const std::vector<glm::vec3>& GetNormals() const { return normals_; }
        const std::vector<std::vector<glm::vec3>>& GetTexCoords() const { return texCoords_; }
        const std::vector<glm::vec3>& GetTangents() const { return tangents_; }
        const std::vector<glm::vec3>& GetBinormals() const { return binormals_; }
        const std::vector<std::vector<glm::vec4>>& GetColors() const { return colors_; }
        const std::vector<std::vector<glm::uvec4>>& GetIndexVectors() const { return indexVectors_; }

        const std::vector<std::uint32_t>& GetIndices() const { return indices_; }

        const glm::mat4& GetRootTransform() const { return rootTransform_; }
        const SceneMeshNode* GetRootNode() const { return rootNode_.get(); }

        const std::vector<MaterialInfo>& GetMaterials() const { return materials_; }
        const MaterialInfo* GetMaterial(unsigned int id) const { return &materials_[id]; }

        template<class VertexType>
        void GetVertices(std::vector<VertexType>& vertices) const;

    protected:
        void SetRootTransform(const glm::mat4& rootTransform) { rootTransform_ = rootTransform; }
        std::vector<glm::vec3>& GetVertices() { return vertices_; }
        std::vector<glm::vec3>& GetNormals() { return normals_; }
        std::vector<std::vector<glm::vec3>>& GetTexCoords() { return texCoords_; }
        std::vector<glm::vec3>& GetTangents() { return tangents_; }
        std::vector<glm::vec3>& GetBinormals() { return binormals_; }
        std::vector<std::vector<glm::vec4>>& GetColors() { return colors_; }
        std::vector<std::vector<glm::uvec4>>& GetIndexVectors() { return indexVectors_; }
        std::vector<std::uint32_t>& GetIndices() { return indices_; }
        std::vector<glm::uvec4>& GetBoneOffsetMatrixIndices() noexcept { return boneOffsetMatrixIndices_; }
        std::vector<glm::vec4>& GetBoneWeigths() noexcept { return boneWeights_; }

        std::vector<glm::mat4>& GetInverseBindPoseMatrices() noexcept { return inverseBindPoseMatrices_; }
        std::vector<Animation>& GetAnimations() noexcept { return animations_; }
        std::vector<std::size_t>& GetBoneParents() noexcept { return boneParent_; }

        void ReserveMesh(unsigned int maxUVChannels, unsigned int maxColorChannels, bool hasTangentSpace,
            unsigned int numVertices, unsigned int numIndices, unsigned int numMaterials);
        MaterialInfo* GetMaterial(unsigned int id) { return &materials_[id]; }
        void AddSubMesh(const std::string& name, unsigned int idxOffset, unsigned int numIndices, unsigned int materialID);
        // void CreateIndexBuffer();

        void CreateSceneNodes(aiNode* rootNode, const std::map<std::string, unsigned int>& boneMap);

    private:
        /** Generates AABB for all bones. */
        void GenerateBoneBoundingBoxes();

        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void save(Archive& ar, const std::uint32_t version) const
        {
            ar(cereal::make_nvp("vertices", vertices_),
                cereal::make_nvp("normals", normals_),
                cereal::make_nvp("texCoords", texCoords_),
                cereal::make_nvp("tangents", tangents_),
                cereal::make_nvp("binormals", binormals_),
                cereal::make_nvp("colors", colors_),
                cereal::make_nvp("boneOffsetMatrixIndices", boneOffsetMatrixIndices_),
                cereal::make_nvp("boneWeigths", boneWeights_),
                cereal::make_nvp("indexVectors", indexVectors_),
                cereal::make_nvp("inverseBindPoseMatrices", inverseBindPoseMatrices_),
                cereal::make_nvp("boneParents", boneParent_),
                cereal::make_nvp("indices", indices_),
                cereal::make_nvp("materials", materials_),
                cereal::make_nvp("subMeshes", subMeshes_),
                cereal::make_nvp("animations", animations_),
                cereal::make_nvp("rootTransform", rootTransform_),
                cereal::make_nvp("rootNode", rootNode_),
                cereal::make_nvp("globalInverse", globalInverse_),
                cereal::make_nvp("boneBoundingBoxes", boneBoundingBoxes_));
        }

        template <class Archive>
        void load(Archive& ar, const std::uint32_t version)
        {
            ar(cereal::make_nvp("vertices", vertices_),
                cereal::make_nvp("normals", normals_),
                cereal::make_nvp("texCoords", texCoords_),
                cereal::make_nvp("tangents", tangents_),
                cereal::make_nvp("binormals", binormals_),
                cereal::make_nvp("colors", colors_),
                cereal::make_nvp("boneOffsetMatrixIndices", boneOffsetMatrixIndices_),
                cereal::make_nvp("boneWeigths", boneWeights_),
                cereal::make_nvp("indexVectors", indexVectors_),
                cereal::make_nvp("inverseBindPoseMatrices", inverseBindPoseMatrices_),
                cereal::make_nvp("boneParents", boneParent_),
                cereal::make_nvp("indices", indices_),
                cereal::make_nvp("materials", materials_),
                cereal::make_nvp("subMeshes", subMeshes_),
                cereal::make_nvp("animations", animations_),
                cereal::make_nvp("rootTransform", rootTransform_),
                cereal::make_nvp("rootNode", rootNode_),
                cereal::make_nvp("globalInverse", globalInverse_),
                cereal::make_nvp("boneBoundingBoxes", boneBoundingBoxes_));
            rootNode_->FlattenNodeTree(nodes_);
        }

        /** Holds all the single points used by the mesh (and its sub-meshes) as points or in vertices. */
        std::vector<glm::vec3> vertices_;
        /** Holds all the single normals used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> normals_;
        /** Holds all the single texture coordinates used by the mesh (and its sub-meshes). */
        std::vector<std::vector<glm::vec3>> texCoords_;
        /** Holds all the single tangents used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> tangents_;
        /** Holds all the single bi-normals used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> binormals_;
        /** Holds all the single colors used by the mesh (and its sub-meshes). */
        std::vector<std::vector<glm::vec4>> colors_;
        /** The indices to bones influencing this vertex (corresponds to boneWeights_). */
        std::vector<glm::uvec4> boneOffsetMatrixIndices_;
        /** Weights, how strong a vertex is influenced by the matrix of the bone. */
        std::vector<glm::vec4> boneWeights_;
        /** Holds integer vectors to be used as indices (similar to boneOffsetMatrixIndices_ but more general). */
        std::vector<std::vector<glm::uvec4>> indexVectors_;

        /** Offset matrices for each bone. */
        std::vector<glm::mat4> inverseBindPoseMatrices_;
        /** Parent of a bone. Stores the parent for each bone in boneOffsetMatrices_. */
        std::vector<std::size_t> boneParent_;

        /** Holds all the indices used by the sub-meshes. */
        std::vector<std::uint32_t> indices_;

        /** The meshes materials. */
        std::vector<MaterialInfo> materials_;
        /** Holds all the meshes sub-meshes. */
        std::vector<std::unique_ptr<SubMesh>> subMeshes_;
        /** Holds all the meshes nodes. */
        std::vector<const SceneMeshNode*> nodes_;
        /** Animations of this mesh */
        std::vector<Animation> animations_;

        /** The root transformation for the meshes. */
        glm::mat4 rootTransform_;
        /** The root scene node. */
        std::unique_ptr<SceneMeshNode> rootNode_;
        /** The global inverse of this mesh. */
        glm::mat4 globalInverse_;
        /** AABB for all bones */
        std::vector<math::AABB3<float>> boneBoundingBoxes_;
    };

    template <class VertexType>
    void MeshInfo::GetVertices(std::vector<VertexType>& vertices) const
    {
        assert(vertices.empty());
        vertices.reserve(vertices_.size());
        for (std::size_t i = 0; i < vertices_.size(); ++i) vertices.emplace_back(this, i);
    }

    /*template <class VTX>
    void Mesh::CreateVertexBuffer()
    {
        try {
            vBuffers_.at(typeid(VTX));
        }
        catch (std::out_of_range e) {
            auto vBuffer = std::make_unique<GLBuffer>(GL_STATIC_DRAW);
            std::vector<VTX> vertices;
            GetVertices(vertices);
            OGL_CALL(glBindBuffer, GL_ARRAY_BUFFER, vBuffer->GetBuffer());
            vBuffer->InitializeData(vertices);
            OGL_CALL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            vBuffers_[typeid(VTX)] = std::move(vBuffer);
        }
    }

    template <class VTX>
    void Mesh::CreateVertexBuffer(const std::vector<VTX>& vertices)
    {
        try {
            vBuffers_.at(typeid(VTX));
        }
        catch (std::out_of_range e) {
            auto vBuffer = std::make_unique<GLBuffer>(GL_STATIC_DRAW);
            OGL_CALL(glBindBuffer, GL_ARRAY_BUFFER, vBuffer->GetBuffer());
            vBuffer->InitializeData(vertices);
            OGL_CALL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            vBuffers_[typeid(VTX)] = std::move(vBuffer);
        }
    }*/
}

CEREAL_CLASS_VERSION(vku::gfx::MeshInfo, 2)
