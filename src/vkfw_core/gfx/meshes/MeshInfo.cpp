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


namespace vkfw_core::gfx {

    /** Default constructor. */
    MeshInfo::MeshInfo() = default;

    /** Copy constructor. */
    MeshInfo::MeshInfo(const MeshInfo& rhs) :
        m_vertices(rhs.m_vertices),
        m_normals(rhs.m_normals),
        m_texCoords(rhs.m_texCoords),
        m_tangents(rhs.m_tangents),
        m_binormals(rhs.m_binormals),
        m_colors(rhs.m_colors),
        m_boneOffsetMatrixIndices(rhs.m_boneOffsetMatrixIndices),
        m_boneWeights(rhs.m_boneWeights),
        m_indexVectors(rhs.m_indexVectors),
        m_inverseBindPoseMatrices(rhs.m_inverseBindPoseMatrices),
        m_boneParent(rhs.m_boneParent),
        m_indices(rhs.m_indices),
        m_materials(rhs.m_materials),
        m_animations(rhs.m_animations),
        m_rootNode(std::make_unique<SceneMeshNode>(*rhs.m_rootNode)),
        m_globalInverse(rhs.m_globalInverse),
        m_boneBoundingBoxes(rhs.m_boneBoundingBoxes)
    {
        for (const auto& submesh : rhs.m_subMeshes) {
            m_subMeshes.emplace_back(submesh);
        }
        m_rootNode->FlattenNodeTree(m_nodes);
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
    MeshInfo::MeshInfo(MeshInfo&& rhs) noexcept
        : m_vertices(std::move(rhs.m_vertices)),
          m_normals(std::move(rhs.m_normals)),
          m_texCoords(std::move(rhs.m_texCoords)),
          m_tangents(std::move(rhs.m_tangents)),
          m_binormals(std::move(rhs.m_binormals)),
          m_colors(std::move(rhs.m_colors)),
          m_boneOffsetMatrixIndices(std::move(rhs.m_boneOffsetMatrixIndices)),
          m_boneWeights(std::move(rhs.m_boneWeights)),
          m_indexVectors(std::move(rhs.m_indexVectors)),
          m_inverseBindPoseMatrices(std::move(rhs.m_inverseBindPoseMatrices)),
          m_boneParent(std::move(rhs.m_boneParent)),
          m_indices(std::move(rhs.m_indices)),
          m_materials(std::move(rhs.m_materials)),
          m_subMeshes(std::move(rhs.m_subMeshes)),
          m_nodes(std::move(rhs.m_nodes)),
          m_animations(std::move(rhs.m_animations)),
          m_rootNode(std::move(rhs.m_rootNode)),
          m_globalInverse(rhs.m_globalInverse),
          m_boneBoundingBoxes(std::move(rhs.m_boneBoundingBoxes))
    {
    }

    /** Default move assignment operator. */
    MeshInfo& MeshInfo::operator=(MeshInfo&& rhs) noexcept
    {
        this->~MeshInfo();
        m_vertices = std::move(rhs.m_vertices);
        m_normals = std::move(rhs.m_normals);
        m_texCoords = std::move(rhs.m_texCoords);
        m_tangents = std::move(rhs.m_tangents);
        m_binormals = std::move(rhs.m_binormals);
        m_colors = std::move(rhs.m_colors);
        m_boneOffsetMatrixIndices = std::move(rhs.m_boneOffsetMatrixIndices);
        m_boneWeights = std::move(rhs.m_boneWeights);
        m_indexVectors = std::move(rhs.m_indexVectors);
        m_inverseBindPoseMatrices = std::move(rhs.m_inverseBindPoseMatrices);
        m_boneParent = std::move(rhs.m_boneParent);
        m_indices = std::move(rhs.m_indices);
        m_materials = std::move(rhs.m_materials);
        m_subMeshes = std::move(rhs.m_subMeshes);
        m_nodes = std::move(rhs.m_nodes);
        m_animations = std::move(rhs.m_animations);
        m_rootNode = std::move(rhs.m_rootNode);
        m_globalInverse = rhs.m_globalInverse;
        m_boneBoundingBoxes = std::move(rhs.m_boneBoundingBoxes);
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
    }

    void MeshInfo::AddSubMesh(const std::string& name, unsigned int idxOffset, unsigned int numIndices, unsigned int materialID)
    {
        m_subMeshes.emplace_back(this, name, idxOffset, numIndices, materialID);
    }

    void MeshInfo::CreateSceneNodes(aiNode* rootNode, const std::map<std::string, unsigned int>& boneMap)
    {
        m_rootNode = std::make_unique<SceneMeshNode>(rootNode, nullptr, boneMap);
        m_rootNode->GenerateBoundingBoxes(*this);
        GenerateBoneBoundingBoxes();
        m_globalInverse = glm::inverse(m_rootNode->GetLocalTransform());
    }

    ///
    /// Generate all BoundingBoxes for the bones.
    ///
    void MeshInfo::GenerateBoneBoundingBoxes()
    {
        if (m_inverseBindPoseMatrices.empty()) {
            return;
        }

        m_boneBoundingBoxes.resize(m_inverseBindPoseMatrices.size());

        bool hasVertexWithoutBone = false;

        for (auto i = 0U; i < m_boneOffsetMatrixIndices.size(); i++) {

            bool vertexHasBone = false;

            for (auto b = 0; b < 4; b++) {
                auto boneI = m_boneOffsetMatrixIndices[i][b];
                auto boneW = m_boneWeights[i][b];
                if (boneW > 0) {
                    vertexHasBone = true;
                    math::AABB3<float>& box = m_boneBoundingBoxes[boneI];
                    box.AddPoint(m_vertices[i]);
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
        m_rootNode->FlattenNodeTree(m_nodes);
        std::map<std::string, std::size_t> nodeIndexMap;
        for (const auto& node : m_nodes) { nodeIndexMap[node->GetName()] = node->GetNodeIndex(); }

        for (auto& animation : m_animations) { animation.FlattenHierarchy(m_nodes.size(), nodeIndexMap); }
    }
}
