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

    class SceneMeshNode
    {
    public:
        SceneMeshNode() : nodeName_(""), parent_(nullptr) { aabb_.minmax[0] = glm::vec3(std::numeric_limits<float>::infinity()); aabb_.minmax[1] = glm::vec3(-std::numeric_limits<float>::infinity()); }
        SceneMeshNode(aiNode* node, const SceneMeshNode* parent, const std::vector<std::unique_ptr<SubMesh>>& meshes);
        SceneMeshNode(const SceneMeshNode& rhs);
        SceneMeshNode& operator=(const SceneMeshNode& rhs);
        SceneMeshNode(SceneMeshNode&& rhs) noexcept;
        SceneMeshNode operator=(SceneMeshNode&& rhs) noexcept;
        ~SceneMeshNode();

        void GetBoundingBox(math::AABB3<float>& aabb, const glm::mat4& transform) const;
        glm::mat4 GetLocalTransform() const { return localTransform_; }
        unsigned int GetNumNodes() const { return static_cast<unsigned int>(children_.size()); }
        const SceneMeshNode* GetChild(unsigned int idx) const { return children_[idx].get(); }
        unsigned int GetNumMeshes() const { return static_cast<unsigned int>(meshes_.size()); }
        const SubMesh* GetMesh(unsigned int idx) const { return meshes_[idx]; }

        void UpdateMeshes(const std::unordered_map<const SubMesh*, const SubMesh*>& meshUpdates);

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            std::vector<std::uint64_t> meshes; meshes.reserve(meshes_.size());
            for (auto m : meshes_) meshes.push_back(reinterpret_cast<std::uint64_t>(m));
            ar(cereal::make_nvp("nodeName", nodeName_),
                cereal::make_nvp("children", children_),
                cereal::make_nvp("meshes", meshes),
                cereal::make_nvp("localTransform", localTransform_),
                cereal::make_nvp("AABB", aabb_));

            meshes_.clear(); meshes_.reserve(meshes.size());
            for (auto m : meshes) meshes_.push_back(reinterpret_cast<const SubMesh*>(m));
            for (auto& c : children_) c->parent_ = this;
        }

        /** The nodes name. */
        std::string nodeName_;
        /** The nodes children. */
        std::vector<std::unique_ptr<SceneMeshNode>> children_;
        /** The meshes in this node. */
        std::vector<const SubMesh*> meshes_;
        /** The local transformation matrix. */
        glm::mat4 localTransform_;
        /** The nodes local AABB. */
        math::AABB3<float> aabb_;
        /** The nodes parent. */
        const SceneMeshNode* parent_;
    };
}

CEREAL_CLASS_VERSION(vku::gfx::SceneMeshNode, 1)
