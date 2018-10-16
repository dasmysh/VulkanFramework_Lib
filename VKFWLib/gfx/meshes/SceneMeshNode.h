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

namespace vku::gfx {

    class MeshInfo;

    class SceneMeshNode
    {
    public:
        SceneMeshNode();
        SceneMeshNode(aiNode* node, const SceneMeshNode* parent, const std::map<std::string, unsigned int>& boneMap);
        SceneMeshNode(const SceneMeshNode& rhs);
        SceneMeshNode& operator=(const SceneMeshNode& rhs);
        SceneMeshNode(SceneMeshNode&& rhs) noexcept;
        SceneMeshNode operator=(SceneMeshNode&& rhs) noexcept;
        ~SceneMeshNode() noexcept;

        /** Returns the local transformation matrix. */
        glm::mat4 GetLocalTransform() const noexcept { return localTransform_; }
        /** Returns the number of children nodes. */
        std::size_t GetNumberOfNodes() const noexcept { return children_.size(); }
        /**
         *  Returns a child node.
         *  @param index the index of the child node.
         */
        const SceneMeshNode* GetChild(std::size_t index) const noexcept { return children_[index].get(); }
        /** Returns the number of sub meshes of the node. */
        std::size_t GetNumberOfSubMeshes() const noexcept { return subMeshIds_.size(); }
        /**
         *  Returns a sub-mesh.
         *  @param index the index of the sub mesh.
         */
        std::size_t GetSubMeshID(std::size_t index) const noexcept { return subMeshIds_[index]; }
        /** Returns the nodes parent. */
        const SceneMeshNode* GetParent() const noexcept { return parent_; }
        /** Returns the node name. */
        const std::string& GetName() const noexcept { return nodeName_; }
        /** Returns the bone index. */
        int GetBoneIndex() const noexcept { return boneIndex_; }
        /** Returns the node index. */
        unsigned int GetNodeIndex() const noexcept { return nodeIndex_; }
        /** Returns the nodes local AABB. */
        const math::AABB3<float>& GetBoundingBox() const noexcept { return aabb_; }
        /** Returns AABB for all sub meshes of the node. */
        const std::vector<math::AABB3<float>>& GetSubMeshBoundingBoxes() const { return subMeshBoundingBoxes_; }
        /** Returns if the AABB is valid. */
        bool IsBoundingBoxValid() const noexcept { return boundingBoxValid_; }

        void FlattenNodeTree(std::vector<const SceneMeshNode*>& nodes);
        bool GenerateBoundingBoxes(const MeshInfo& mesh);

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            ar(cereal::make_nvp("nodeName", nodeName_),
                cereal::make_nvp("children", children_),
                cereal::make_nvp("subMeshIDs", subMeshIds_),
                cereal::make_nvp("localTransform", localTransform_),
                cereal::make_nvp("boneIndex", boneIndex_),
                cereal::make_nvp("nodeIndex", nodeIndex_),
                cereal::make_nvp("AABB", aabb_),
                cereal::make_nvp("subMeshBoundingBoxes", subMeshBoundingBoxes_),
                cereal::make_nvp("boundingBoxValid", boundingBoxValid_));

            for (auto& c : children_) c->parent_ = this;
        }

        /** The nodes name. */
        std::string nodeName_;
        /** The nodes children. */
        std::vector<std::unique_ptr<SceneMeshNode>> children_;
        /** Meshes in this node. */
        std::vector<std::size_t> subMeshIds_;
        /** The local transformation matrix. */
        glm::mat4 localTransform_;
        /** The nodes parent. */
        const SceneMeshNode* parent_;
        /** Bone index. */
        int boneIndex_;
        /** Node index. */
        unsigned int nodeIndex_;
        /** The nodes local AABB. */
        math::AABB3<float> aabb_;
        /** Bounding boxes for this nodes sub meshes. */
        std::vector<math::AABB3<float>> subMeshBoundingBoxes_;
        /** Flag if the bounding box is valid. */
        bool boundingBoxValid_ = false;
    };
}

CEREAL_CLASS_VERSION(vku::gfx::SceneMeshNode, 1)
