/**
 * @file   SceneMeshNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Implementation of a (sub-) mesh node in a scene.
 */

#include "gfx/meshes/SceneMeshNode.h"
#include <assimp/scene.h>
#include "gfx/meshes/SubMesh.h"
#include "assimp_convert_helpers.h"
#include <core/math/transforms.h>
#include "gfx/meshes/MeshInfo.h"


namespace vkfw_core::gfx {

    SceneMeshNode::SceneMeshNode() :
        m_localTransform{ glm::mat4{1.0} }, m_parent{ nullptr }
    {
        m_aabb.m_minmax[0] = glm::vec3(std::numeric_limits<float>::infinity());
        m_aabb.m_minmax[1] = glm::vec3(-std::numeric_limits<float>::infinity());
    }

    SceneMeshNode::SceneMeshNode(aiNode* node, const SceneMeshNode* parent, const std::map<std::string, unsigned int>& boneMap) :
        m_nodeName(node->mName.C_Str()),
        m_localTransform{ AiMatrixToGLM(node->mTransformation) },
        m_parent{ parent },
        m_boneIndex{ -1 }
    {
        m_localTransform[3][3] = 1.0; // <- Making sure this is one

        m_aabb.m_minmax[0] = glm::vec3(std::numeric_limits<float>::infinity());
        m_aabb.m_minmax[1] = glm::vec3(-std::numeric_limits<float>::infinity());

        auto numMeshes = static_cast<std::size_t>(node->mNumMeshes);
        auto numChildren = static_cast<std::size_t>(node->mNumChildren);
        for (std::size_t i = 0; i < numMeshes; ++i) { m_subMeshIds.push_back(node->mMeshes[i]); } // NOLINT
        for (std::size_t i = 0; i < numChildren; ++i) {
            m_children.push_back(std::make_unique<SceneMeshNode>(node->mChildren[i], this, boneMap)); // NOLINT
        }

        auto nodeBone = boneMap.find(m_nodeName);
        if (nodeBone != boneMap.end()) { m_boneIndex = nodeBone->second; }
    }

    SceneMeshNode::SceneMeshNode(const SceneMeshNode& rhs)
        : m_nodeName(rhs.m_nodeName),
          m_subMeshIds(rhs.m_subMeshIds),
          m_localTransform(rhs.m_localTransform),
          m_parent(rhs.m_parent),
          m_boneIndex(rhs.m_boneIndex),
          m_nodeIndex(rhs.m_nodeIndex),
          m_aabb(rhs.m_aabb),
          m_subMeshBoundingBoxes(rhs.m_subMeshBoundingBoxes),
          m_boundingBoxValid(rhs.m_boundingBoxValid),
          m_hasMeshes(rhs.m_hasMeshes)
    {
        m_children.resize(rhs.m_children.size());
        for (std::size_t i = 0; i < m_children.size(); ++i) {
            m_children[i] = std::make_unique<SceneMeshNode>(*rhs.m_children[i]);
        }
    }

    SceneMeshNode::SceneMeshNode(SceneMeshNode&& rhs) noexcept
        : m_nodeName(std::move(rhs.m_nodeName)),
          m_children(std::move(rhs.m_children)),
          m_subMeshIds(std::move(rhs.m_subMeshIds)),
          m_localTransform(rhs.m_localTransform),
          m_parent(rhs.m_parent),
          m_boneIndex(rhs.m_boneIndex),
          m_nodeIndex(rhs.m_nodeIndex),
          m_aabb(rhs.m_aabb),
          m_subMeshBoundingBoxes(std::move(rhs.m_subMeshBoundingBoxes)),
          m_boundingBoxValid(rhs.m_boundingBoxValid),
          m_hasMeshes(rhs.m_hasMeshes)
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

    SceneMeshNode& SceneMeshNode::operator=(SceneMeshNode&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~SceneMeshNode();
            m_nodeName = std::move(rhs.m_nodeName);
            m_children = std::move(rhs.m_children);
            m_subMeshIds = std::move(rhs.m_subMeshIds);
            m_localTransform = rhs.m_localTransform;
            m_parent = rhs.m_parent;
            m_aabb = rhs.m_aabb;
            m_boneIndex = rhs.m_boneIndex;
            m_nodeIndex = rhs.m_nodeIndex;
            m_subMeshBoundingBoxes = std::move(rhs.m_subMeshBoundingBoxes);
            m_boundingBoxValid = rhs.m_boundingBoxValid;
            m_hasMeshes = rhs.m_hasMeshes;
        }
        return *this;
    }

    SceneMeshNode::~SceneMeshNode() noexcept = default;

    void SceneMeshNode::FlattenNodeTree(std::vector<const SceneMeshNode*>& nodes) { FlattenNodeTreeInternal(nodes); }

    bool SceneMeshNode::FlattenNodeTreeInternal(std::vector<const SceneMeshNode*>& nodes)
    {
        m_hasMeshes = !m_subMeshIds.empty();
        m_nodeIndex = static_cast<unsigned int>(nodes.size());
        nodes.push_back(this);
        for (const auto& child : m_children) { m_hasMeshes = child->FlattenNodeTreeInternal(nodes) || m_hasMeshes; }
        return m_hasMeshes;
    }

    bool SceneMeshNode::GenerateBoundingBoxes(const MeshInfo& mesh)
    {
        bool bbValid = false;
        m_aabb.SetMin(glm::vec3(std::numeric_limits<float>::max()));
        m_aabb.SetMax(glm::vec3(std::numeric_limits<float>::lowest()));
        for (auto subMeshId : m_subMeshIds) {
            math::AABB3<float> subMeshBoundingBox;
            auto firstIdx = mesh.GetSubMeshes()[subMeshId].GetIndexOffset();
            auto lastIdx = firstIdx + mesh.GetSubMeshes()[subMeshId].GetNumberOfIndices();
            for (auto i = firstIdx; i < lastIdx; ++i) {
                auto vertexIndex = mesh.GetIndices()[i];
                subMeshBoundingBox.AddPoint(glm::vec3(m_localTransform * glm::vec4(mesh.GetVertices()[vertexIndex], 1.0f)));
                bbValid = true;
            }
            m_subMeshBoundingBoxes.push_back(subMeshBoundingBox);
            m_aabb = m_aabb.Union(subMeshBoundingBox);
        }

        for (auto& child : m_children) {
            bool childValid = child->GenerateBoundingBoxes(mesh);
            if (childValid) {
                m_aabb = m_aabb.Union(child->GetBoundingBox().NewFromTransform(m_localTransform));
                bbValid = bbValid || childValid;
            }
        }
        m_boundingBoxValid = bbValid;
        return bbValid;
    }
}
