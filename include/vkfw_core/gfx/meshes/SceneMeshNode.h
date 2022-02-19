/**
 * @file   SceneMeshNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Declaration of a (sub-) mesh node in a scene.
 */

#pragma once

#include "main.h"
#include <core/math/primitives.h>
#include <unordered_map>
#include <cereal/cereal.hpp>
#include "SubMesh.h"

struct aiNode;

namespace vkfw_core::gfx {

    class MeshInfo;

    class SceneMeshNode
    {
    public:
        SceneMeshNode();
        SceneMeshNode(aiNode* node, const SceneMeshNode* parent, const std::map<std::string, unsigned int>& boneMap);
        SceneMeshNode(const SceneMeshNode& rhs);
        SceneMeshNode& operator=(const SceneMeshNode& rhs);
        SceneMeshNode(SceneMeshNode&& rhs) noexcept;
        SceneMeshNode& operator=(SceneMeshNode&& rhs) noexcept;
        ~SceneMeshNode() noexcept;

        /** Returns the local transformation matrix. */
        [[nodiscard]] glm::mat4 GetLocalTransform() const noexcept { return m_localTransform; }
        /** Returns the number of children nodes. */
        [[nodiscard]] std::size_t GetNumberOfNodes() const noexcept { return m_children.size(); }
        /**
         *  Returns a child node.
         *  @param index the index of the child node.
         */
        [[nodiscard]] const SceneMeshNode* GetChild(std::size_t index) const noexcept { return m_children[index].get(); }
        /** Returns the number of sub meshes of the node. */
        [[nodiscard]] std::size_t GetNumberOfSubMeshes() const noexcept { return m_subMeshIds.size(); }
        /**
         *  Returns a sub-mesh.
         *  @param index the index of the sub mesh.
         */
        [[nodiscard]] std::size_t GetSubMeshID(std::size_t index) const noexcept { return m_subMeshIds[index]; }
        /** Returns the nodes parent. */
        [[nodiscard]] const SceneMeshNode* GetParent() const noexcept { return m_parent; }
        /** Returns the node name. */
        [[nodiscard]] const std::string& GetName() const noexcept { return m_nodeName; }
        /** Returns the bone index. */
        [[nodiscard]] int GetBoneIndex() const noexcept { return m_boneIndex; }
        /** Returns the node index. */
        [[nodiscard]] unsigned int GetNodeIndex() const noexcept { return m_nodeIndex; }
        /** Returns the nodes local AABB. */
        [[nodiscard]] const math::AABB3<float>& GetBoundingBox() const noexcept { return m_aabb; }
        /** Returns AABB for all sub meshes of the node. */
        [[nodiscard]] const std::vector<math::AABB3<float>>& GetSubMeshBoundingBoxes() const
        {
            return m_subMeshBoundingBoxes;
        }
        /** Returns if the AABB is valid. */
        [[nodiscard]] bool IsBoundingBoxValid() const noexcept { return m_boundingBoxValid; }
        /** Returns if the subtree of this node has meshes. */
        [[nodiscard]] bool HasMeshes() const noexcept { return m_hasMeshes; }

        void FlattenNodeTree(std::vector<const SceneMeshNode*>& nodes);
        bool GenerateBoundingBoxes(const MeshInfo& mesh);

    private:
        /**
         *  Flattens the tree of the node and its child nodes to a vector and checks if the subtree has meshes.
         *  @param nodes list of nodes that will contain the flattened tree.
         *  @return whether the subtree has meshes.
         */
        bool FlattenNodeTreeInternal(std::vector<const SceneMeshNode*>& nodes);

        /** Needed for serialization */
        friend class cereal::access;

        template<class Archive> void serialize(Archive& ar, const std::uint32_t) // NOLINT
        {
            ar(cereal::make_nvp("nodeName", m_nodeName),
                cereal::make_nvp("children", m_children),
                cereal::make_nvp("subMeshIDs", m_subMeshIds),
                cereal::make_nvp("localTransform", m_localTransform),
                cereal::make_nvp("boneIndex", m_boneIndex),
                cereal::make_nvp("nodeIndex", m_nodeIndex),
                cereal::make_nvp("AABB", m_aabb),
                cereal::make_nvp("subMeshBoundingBoxes", m_subMeshBoundingBoxes),
                cereal::make_nvp("boundingBoxValid", m_boundingBoxValid));

            for (auto& c : m_children) { c->m_parent = this; }
        }

        /** The nodes name. */
        std::string m_nodeName;
        /** The nodes children. */
        std::vector<std::unique_ptr<SceneMeshNode>> m_children;
        /** Meshes in this node. */
        std::vector<std::size_t> m_subMeshIds;
        /** The local transformation matrix. */
        glm::mat4 m_localTransform;
        /** The nodes parent. */
        const SceneMeshNode* m_parent;
        /** Bone index. */
        int m_boneIndex = -1;
        /** Node index. */
        unsigned int m_nodeIndex = 0;
        /** The nodes local AABB. */
        math::AABB3<float> m_aabb;
        /** Bounding boxes for this nodes sub meshes. */
        std::vector<math::AABB3<float>> m_subMeshBoundingBoxes;
        /** Flag if the bounding box is valid. */
        bool m_boundingBoxValid = false;
        /** Flag if the subtree of this node contains any meshes. */
        bool m_hasMeshes = false;
    };
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::SceneMeshNode, 2)
