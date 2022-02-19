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
#include "core/serialization_helper.h"
#include "core/concepts.h"
#include <cereal/cereal.hpp>

struct aiNode;

namespace vkfw_core::gfx {

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

        /**
         *  Accessor to the meshes sub-meshes. This can be used to render more complicated meshes (with multiple sets
         *  of texture coordinates).
         */
        std::vector<SubMesh>& GetSubMeshes() noexcept { return m_subMeshes; }
        /** Const accessor to the meshes sub-meshes. */
        [[nodiscard]] const std::vector<SubMesh>& GetSubMeshes() const noexcept { return m_subMeshes; }
        /** Accessor to the nodes of the mesh. */
        [[nodiscard]] const std::vector<const SceneMeshNode*>& GetNodes() const noexcept { return m_nodes; }
        /** Returns the root node of the mesh. */
        [[nodiscard]] const SceneMeshNode* GetRootNode() const noexcept { return m_rootNode.get(); }

        [[nodiscard]] const std::vector<glm::vec3>& GetVertices() const { return m_vertices; }
        [[nodiscard]] const std::vector<glm::vec3>& GetNormals() const { return m_normals; }
        [[nodiscard]] const std::vector<std::vector<glm::vec3>>& GetTexCoords() const { return m_texCoords; }
        [[nodiscard]] const std::vector<glm::vec3>& GetTangents() const { return m_tangents; }
        [[nodiscard]] const std::vector<glm::vec3>& GetBinormals() const { return m_binormals; }
        [[nodiscard]] const std::vector<std::vector<glm::vec4>>& GetColors() const { return m_colors; }
        [[nodiscard]] const std::vector<std::vector<glm::uvec4>>& GetIndexVectors() const { return m_indexVectors; }

        [[nodiscard]] const std::vector<std::uint32_t>& GetIndices() const noexcept { return m_indices; }

        [[nodiscard]] const std::vector<Animation>& GetAnimations() const noexcept { return m_animations; }

        /** Returns the offset matrices for all bones. */
        [[nodiscard]] const std::vector<glm::mat4>& GetInverseBindPoseMatrices() const noexcept
        {
            return m_inverseBindPoseMatrices;
        }
        /** Returns the AABB for all bones. */
        [[nodiscard]] const std::vector<math::AABB3<float>>& GetBoneBoundingBoxes() const noexcept
        {
            return m_boneBoundingBoxes;
        }
        /**
         *  Returns the parent bone of any given bone.
         *  @param boneIndex index of the child bone.
         */
        [[nodiscard]] std::size_t GetParentBone(std::size_t boneIndex) const { return m_boneParent[boneIndex]; }
        /** Returns the number of bones used by the mesh. */
        [[nodiscard]] std::size_t GetNumberOfBones() const noexcept { return m_inverseBindPoseMatrices.size(); }

        /** Returns the global inverse matrix of the mesh. */
        [[nodiscard]] glm::mat4 GetGlobalInverse() const { return m_globalInverse; }

        [[nodiscard]] const std::vector<std::unique_ptr<MaterialInfo>>& GetMaterials() const { return m_materials; }
        [[nodiscard]] const MaterialInfo* GetMaterial(unsigned int id) const { return m_materials[id].get(); }

        template<class VertexType>
        void GetVertices(std::vector<VertexType>& vertices) const;

    protected:
        std::vector<glm::vec3>& GetVertices() { return m_vertices; }
        std::vector<glm::vec3>& GetNormals() { return m_normals; }
        std::vector<std::vector<glm::vec3>>& GetTexCoords() { return m_texCoords; }
        std::vector<glm::vec3>& GetTangents() { return m_tangents; }
        std::vector<glm::vec3>& GetBinormals() { return m_binormals; }
        std::vector<std::vector<glm::vec4>>& GetColors() { return m_colors; }
        std::vector<std::vector<glm::uvec4>>& GetIndexVectors() { return m_indexVectors; }
        std::vector<std::uint32_t>& GetIndices() { return m_indices; }
        std::vector<glm::uvec4>& GetBoneOffsetMatrixIndices() noexcept { return m_boneOffsetMatrixIndices; }
        std::vector<glm::vec4>& GetBoneWeigths() noexcept { return m_boneWeights; }

        std::vector<glm::mat4>& GetInverseBindPoseMatrices() noexcept { return m_inverseBindPoseMatrices; }
        std::vector<std::size_t>& GetBoneParents() noexcept { return m_boneParent; }
        std::vector<Animation>& GetAnimations() noexcept { return m_animations; }
        /** Returns the AABB for all bones. */
        std::vector<math::AABB3<float>>& GetBoneBoundingBoxes() noexcept { return m_boneBoundingBoxes; }

        template<vkfw_core::Material MaterialType>
        void ReserveMesh(unsigned int maxUVChannels, unsigned int maxColorChannels, bool hasTangentSpace,
                         unsigned int numVertices, unsigned int numIndices, unsigned int numMaterials);

        MaterialInfo* GetMaterial(std::size_t id) { return m_materials[id].get(); }
        void AddSubMesh(const std::string& name, unsigned int idxOffset, unsigned int numIndices, unsigned int materialID);
        // void CreateIndexBuffer();

        void CreateSceneNodes(aiNode* rootNode, const std::map<std::string, unsigned int>& boneMap);
        /** Flattens all hierarchies. */
        void FlattenHierarchies();

    private:
        /** Generates AABB for all bones. */
        void GenerateBoneBoundingBoxes();

        /** Needed for serialization */
        friend class cereal::access;

        template<class Archive> void save(Archive& ar, const std::uint32_t) const // NOLINT
        {
            ar(cereal::make_nvp("vertices", m_vertices),
                cereal::make_nvp("normals", m_normals),
                cereal::make_nvp("texCoords", m_texCoords),
                cereal::make_nvp("tangents", m_tangents),
                cereal::make_nvp("binormals", m_binormals),
                cereal::make_nvp("colors", m_colors),
                cereal::make_nvp("boneOffsetMatrixIndices", m_boneOffsetMatrixIndices),
                cereal::make_nvp("boneWeigths", m_boneWeights),
                cereal::make_nvp("indexVectors", m_indexVectors),
                cereal::make_nvp("inverseBindPoseMatrices", m_inverseBindPoseMatrices),
                cereal::make_nvp("boneParents", m_boneParent),
                cereal::make_nvp("indices", m_indices),
                cereal::make_nvp("materials", m_materials),
                cereal::make_nvp("subMeshes", m_subMeshes),
                cereal::make_nvp("animations", m_animations),
                cereal::make_nvp("rootNode", m_rootNode),
                cereal::make_nvp("globalInverse", m_globalInverse),
                cereal::make_nvp("boneBoundingBoxes", m_boneBoundingBoxes));
        }

        template<class Archive> void load(Archive& ar, [[maybe_unused]] const std::uint32_t version) // NOLINT
        {
            ar(cereal::make_nvp("vertices", m_vertices), cereal::make_nvp("normals", m_normals),
               cereal::make_nvp("texCoords", m_texCoords), cereal::make_nvp("tangents", m_tangents),
               cereal::make_nvp("binormals", m_binormals), cereal::make_nvp("colors", m_colors),
               cereal::make_nvp("boneOffsetMatrixIndices", m_boneOffsetMatrixIndices),
               cereal::make_nvp("boneWeigths", m_boneWeights), cereal::make_nvp("indexVectors", m_indexVectors),
               cereal::make_nvp("inverseBindPoseMatrices", m_inverseBindPoseMatrices),
               cereal::make_nvp("boneParents", m_boneParent), cereal::make_nvp("indices", m_indices),
               cereal::make_nvp("materials", m_materials), cereal::make_nvp("subMeshes", m_subMeshes),
               cereal::make_nvp("animations", m_animations), cereal::make_nvp("rootNode", m_rootNode),
               cereal::make_nvp("globalInverse", m_globalInverse),
               cereal::make_nvp("boneBoundingBoxes", m_boneBoundingBoxes));
            m_rootNode->FlattenNodeTree(m_nodes);
        }

        /** Holds all the single points used by the mesh (and its sub-meshes) as points or in vertices. */
        std::vector<glm::vec3> m_vertices;
        /** Holds all the single normals used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> m_normals;
        /** Holds all the single texture coordinates used by the mesh (and its sub-meshes). */
        std::vector<std::vector<glm::vec3>> m_texCoords;
        /** Holds all the single tangents used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> m_tangents;
        /** Holds all the single bi-normals used by the mesh (and its sub-meshes). */
        std::vector<glm::vec3> m_binormals;
        /** Holds all the single colors used by the mesh (and its sub-meshes). */
        std::vector<std::vector<glm::vec4>> m_colors;
        /** The indices to bones influencing this vertex (corresponds to m_boneWeights). */
        std::vector<glm::uvec4> m_boneOffsetMatrixIndices;
        /** Weights, how strong a vertex is influenced by the matrix of the bone. */
        std::vector<glm::vec4> m_boneWeights;
        /** Holds integer vectors to be used as indices (similar to m_boneOffsetMatrixIndices but more general). */
        std::vector<std::vector<glm::uvec4>> m_indexVectors;

        /** Offset matrices for each bone. */
        std::vector<glm::mat4> m_inverseBindPoseMatrices;
        /** Parent of a bone. Stores the parent for each bone in m_boneOffsetMatrices. */
        std::vector<std::size_t> m_boneParent;

        /** Holds all the indices used by the sub-meshes. */
        std::vector<std::uint32_t> m_indices;

        /** The meshes materials. */
        std::vector<std::unique_ptr<MaterialInfo>> m_materials;
        /** Holds all the meshes sub-meshes. */
        std::vector<SubMesh> m_subMeshes;
        /** Holds all the meshes nodes. */
        std::vector<const SceneMeshNode*> m_nodes;
        /** Animations of this mesh */
        std::vector<Animation> m_animations;

        /** The root scene node. */
        std::unique_ptr<SceneMeshNode> m_rootNode;
        /** The global inverse of this mesh. */
        glm::mat4 m_globalInverse = glm::mat4{1.0f};
        /** AABB for all bones */
        std::vector<math::AABB3<float>> m_boneBoundingBoxes;
    };

    template <class VertexType>
    void MeshInfo::GetVertices(std::vector<VertexType>& vertices) const
    {
        assert(vertices.empty());
        vertices.reserve(m_vertices.size());
        for (std::size_t i = 0; i < m_vertices.size(); ++i) vertices.emplace_back(this, i);
    }

    /**
     *  Reserves memory to create the mesh.
     *  @param maxUVChannels the maximum number of texture coordinates in a single sub-mesh vertex.
     *  @param maxColorChannels the maximum number of colors in a single sub-mesh vertex.
     *  @param numVertices the number of vertices in the mesh.
     *  @param numIndices the number of indices in the mesh.
     */
    template<vkfw_core::Material MaterialType>
    inline void MeshInfo::ReserveMesh(unsigned int maxUVChannels, unsigned int maxColorChannels, bool hasTangentSpace,
                                      unsigned int numVertices, unsigned int numIndices, unsigned int numMaterials)
    {
        m_vertices.resize(numVertices);
        m_normals.resize(numVertices);
        m_texCoords.resize(maxUVChannels);
        for (auto& texCoords : m_texCoords) { texCoords.resize(numVertices); }
        if (hasTangentSpace) {
            m_tangents.resize(numVertices);
            m_binormals.resize(numVertices);
        }
        m_colors.resize(maxColorChannels);
        for (auto& colors : m_colors) { colors.resize(numVertices); }
        m_indices.resize(numIndices);
        m_materials.resize(numMaterials);
        for (auto& material : m_materials) { material = std::make_unique<MaterialType>(); }
    }
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::MeshInfo, 4)
