/**
 * @file   SceneMeshNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Implementation of a (sub-) mesh node in a scene.
 */

#include "SceneMeshNode.h"
#include <assimp/scene.h>
#include "SubMesh.h"
#include "assimp_convert_helpers.h"
#include <core/math/transforms.h>
#include "MeshInfo.h"


namespace vku::gfx {

    SceneMeshNode::SceneMeshNode() :
        nodeName_{}, localTransform_{ glm::mat4{1.0} }, parent_{ nullptr }
    {
        aabb_.minmax_[0] = glm::vec3(std::numeric_limits<float>::infinity());
        aabb_.minmax_[1] = glm::vec3(-std::numeric_limits<float>::infinity());
    }

    SceneMeshNode::SceneMeshNode(aiNode* node, const SceneMeshNode* parent, const std::map<std::string, unsigned int>& boneMap) :
        nodeName_(node->mName.C_Str()),
        localTransform_{ AiMatrixToGLM(node->mTransformation) },
        parent_{ parent },
        boneIndex_{ -1 }
    {
        aabb_.minmax_[0] = glm::vec3(std::numeric_limits<float>::infinity());
        aabb_.minmax_[1] = glm::vec3(-std::numeric_limits<float>::infinity());

        auto numMeshes = static_cast<std::size_t>(node->mNumMeshes);
        auto numChildren = static_cast<std::size_t>(node->mNumChildren);
        for (std::size_t i = 0; i < numMeshes; ++i) subMeshIds_.push_back(node->mMeshes[i]);
        for (std::size_t i = 0; i < numChildren; ++i) children_.push_back(std::make_unique<SceneMeshNode>(node->mChildren[i], this, boneMap));

        auto nodeBone = boneMap.find(nodeName_);
        if (nodeBone != boneMap.end()) boneIndex_ = nodeBone->second;
    }

    SceneMeshNode::SceneMeshNode(const SceneMeshNode& rhs) :
        nodeName_(rhs.nodeName_),
        subMeshIds_(rhs.subMeshIds_),
        localTransform_(rhs.localTransform_),
        parent_(rhs.parent_),
        aabb_(rhs.aabb_),
        boneIndex_(rhs.boneIndex_),
        nodeIndex_(rhs.nodeIndex_),
        subMeshBoundingBoxes_(rhs.subMeshBoundingBoxes_),
        boundingBoxValid_(rhs.boundingBoxValid_)
    {
        children_.resize(rhs.children_.size());
        for (std::size_t i = 0; i < children_.size(); ++i) children_[i] = std::make_unique<SceneMeshNode>(*rhs.children_[i]);
    }

    SceneMeshNode::SceneMeshNode(SceneMeshNode&& rhs) noexcept :
        nodeName_(std::move(rhs.nodeName_)),
        children_(std::move(rhs.children_)),
        subMeshIds_(std::move(rhs.subMeshIds_)),
        localTransform_(std::move(rhs.localTransform_)),
        parent_(std::move(rhs.parent_)),
        aabb_(std::move(rhs.aabb_)),
        boneIndex_(std::move(rhs.boneIndex_)),
        nodeIndex_(std::move(rhs.nodeIndex_)),
        subMeshBoundingBoxes_(std::move(rhs.subMeshBoundingBoxes_)),
        boundingBoxValid_(std::move(rhs.boundingBoxValid_))
    {

    }

    SceneMeshNode& SceneMeshNode::operator=(const SceneMeshNode& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    SceneMeshNode SceneMeshNode::operator=(SceneMeshNode&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~SceneMeshNode();
            nodeName_ = std::move(rhs.nodeName_);
            children_ = std::move(rhs.children_);
            subMeshIds_ = std::move(rhs.subMeshIds_);
            localTransform_ = std::move(rhs.localTransform_);
            parent_ = std::move(rhs.parent_);
            aabb_ = std::move(rhs.aabb_);
            boneIndex_ = std::move(rhs.boneIndex_);
            nodeIndex_ = std::move(rhs.nodeIndex_);
            subMeshBoundingBoxes_ = std::move(rhs.subMeshBoundingBoxes_);
            boundingBoxValid_ = std::move(rhs.boundingBoxValid_);
        }
        return *this;
    }

    SceneMeshNode::~SceneMeshNode() = default;

    void SceneMeshNode::FlattenNodeTree(std::vector<const SceneMeshNode*>& nodes)
    {
        nodeIndex_ = static_cast<unsigned int>(nodes.size());
        nodes.push_back(this);
        for (const auto& child : children_) child->FlattenNodeTree(nodes);
    }

    bool SceneMeshNode::GenerateBoundingBoxes(const MeshInfo& mesh)
    {
        bool bbValid = false;
        aabb_.SetMin(glm::vec3(std::numeric_limits<float>::max()));
        aabb_.SetMax(glm::vec3(std::numeric_limits<float>::lowest()));
        for (auto subMeshId : subMeshIds_) {
            math::AABB3<float> subMeshBoundingBox;
            auto firstIdx = mesh.GetSubMesh(subMeshId)->GetIndexOffset();
            auto lastIdx = firstIdx + mesh.GetSubMesh(subMeshId)->GetNumberOfIndices();
            for (auto i = firstIdx; i < lastIdx; ++i) {
                auto vertexIndex = mesh.GetIndices()[i];
                subMeshBoundingBox.AddPoint(glm::vec3(localTransform_ * glm::vec4(mesh.GetVertices()[vertexIndex], 1.0f)));
                bbValid = true;
            }
            subMeshBoundingBoxes_.push_back(subMeshBoundingBox);
            aabb_ = aabb_.Union(subMeshBoundingBox);
        }

        for (auto& child : children_) {
            bool childValid = child->GenerateBoundingBoxes(mesh);
            if (childValid) {
                aabb_ = aabb_.Union(child->GetBoundingBox().NewFromTransform(localTransform_));
                bbValid = bbValid || childValid;
            }
        }
        boundingBoxValid_ = bbValid;
        return bbValid;
    }
}
