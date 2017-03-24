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
        const SubMesh* GetSubMesh(unsigned int id) const { return subMeshes_[id].get(); }

        const std::vector<glm::vec3>& GetVertices() const { return vertices_; }
        const std::vector<glm::vec3>& GetNormals() const { return normals_; }
        const std::vector<std::vector<glm::vec3>>& GetTexCoords() const { return texCoords_; }
        const std::vector<glm::vec3>& GetTangents() const { return tangents_; }
        const std::vector<glm::vec3>& GetBinormals() const { return binormals_; }
        const std::vector<std::vector<glm::vec4>>& GetColors() const { return colors_; }
        const std::vector<std::vector<unsigned int>>& GetIds() const { return ids_; }

        const std::vector<std::uint32_t>& GetIndices() const { return indices_; }

        const glm::mat4& GetRootTransform() const { return rootTransform_; }
        const SceneMeshNode* GetRootNode() const { return rootNode_.get(); }

        const std::vector<MaterialInfo>& GetMaterials() const { return materials_; }
        const MaterialInfo* GetMaterial(unsigned int id) const { return &materials_[id]; }
        // const DeviceBuffer* GetIndexBuffer() const { return iBuffer_.get(); }

        template<class VertexType>
        void GetVertices(std::vector<VertexType>& vertices) const;
        /*template<class VTX>
        void CreateVertexBuffer();
        template<class VTX>
        void CreateVertexBuffer(const std::vector<VTX>& vertices);*/

        // template<class VTX> DeviceBuffer* GetVertexBuffer() { return vBuffers_.at(typeid(VTX)).get(); }
        // template<class VTX> const DeviceBuffer* GetVertexBuffer() const { return vBuffers_.at(typeid(VTX)).get(); }
        // virtual std::string GetFullFilename() const { return ""; };

        // TODO: add serialization. [3/22/2017 Sebastian Maisch]
        /*void write(std::ofstream& ofs) const;
        bool read(std::ifstream& ifs, TextureManager& texMan);*/

    protected:
        void SetRootTransform(const glm::mat4& rootTransform) { rootTransform_ = rootTransform; }
        std::vector<glm::vec3>& GetVertices() { return vertices_; }
        std::vector<glm::vec3>& GetNormals() { return normals_; }
        std::vector<std::vector<glm::vec3>>& GetTexCoords() { return texCoords_; }
        std::vector<glm::vec3>& GetTangents() { return tangents_; }
        std::vector<glm::vec3>& GetBinormals() { return binormals_; }
        std::vector<std::vector<glm::vec4>>& GetColors() { return colors_; }
        std::vector<std::vector<unsigned int>>& GetIds() { return ids_; }
        std::vector<std::uint32_t>& GetIndices() { return indices_; }

        void ReserveMesh(unsigned int maxUVChannels, unsigned int maxColorChannels,
            unsigned int numVertices, unsigned int numIndices, unsigned int numMaterials);
        MaterialInfo* GetMaterial(unsigned int id) { return &materials_[id]; }
        void AddSubMesh(const std::string& name, unsigned int idxOffset, unsigned int numIndices, unsigned int materialID);
        // void CreateIndexBuffer();

        void CreateSceneNodes(aiNode* rootNode);

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            ar(cereal::make_nvp("vertices", vertices_),
                cereal::make_nvp("normals", normals_),
                cereal::make_nvp("texCoords", texCoords_),
                cereal::make_nvp("tangents", tangents_),
                cereal::make_nvp("binormals", binormals_),
                cereal::make_nvp("colors", colors_),
                cereal::make_nvp("ids", ids_),
                cereal::make_nvp("indices", indices_),
                cereal::make_nvp("rootTransform", rootTransform_),
                cereal::make_nvp("rootNode", rootNode_),
                cereal::make_nvp("materials", materials_),
                cereal::make_nvp("subMeshes", subMeshes_));

            std::unordered_map<const SubMesh*, const SubMesh*> meshUpdates;
            for (const auto& m : subMeshes_) meshUpdates[reinterpret_cast<const SubMesh*>(m->GetSerializationID())] = m.get();
            rootNode_->UpdateMeshes(meshUpdates);
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
        /** Holds the indices that are part of each vertex. */
        std::vector<std::vector<std::uint32_t>> ids_;
        /** Holds all the indices used by the sub-meshes. */
        std::vector<std::uint32_t> indices_;

        /** Holds a map of different vertex buffers for each vertex type. */
        // std::unordered_map<std::type_index, std::unique_ptr<DeviceBuffer>> vBuffers_;
        /** Holds the meshes index buffer. */
        // std::unique_ptr<DeviceBuffer> iBuffer_;

        /** The root transformation for the meshes. */
        glm::mat4 rootTransform_;
        /** The root scene node. */
        std::unique_ptr<SceneMeshNode> rootNode_;

        /** The meshes materials. */
        std::vector<MaterialInfo> materials_;

        /** Holds all the meshes sub-meshes (as an array for fast iteration during rendering). */
        std::vector<std::unique_ptr<SubMesh>> subMeshes_;
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

CEREAL_CLASS_VERSION(vku::gfx::MeshInfo, 1)
