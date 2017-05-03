/**
 * @file   MeshInfo.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.13
 *
 * @brief  Contains the implementation of the MeshInfo class.
 */

#define GLM_FORCE_SWIZZLE

#include "MeshInfo.h"
#include "SubMesh.h"
#include "gfx/Material.h"
#include "core/serialization_helper.h"
#include "SceneMeshNode.h"
#include <gfx/vk/buffers/DeviceBuffer.h>
#include <fstream>

#undef min
#undef max


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
        ids_(rhs.ids_),
        indices_(rhs.indices_),
        rootTransform_(rhs.rootTransform_),
        rootNode_(std::make_unique<SceneMeshNode>(*rhs.rootNode_)),
        materials_(rhs.materials_)
    {
        for (const auto& submesh : rhs.subMeshes_) {
            auto newSubMesh = std::make_unique<SubMesh>(*submesh);
            subMeshes_.push_back(std::move(newSubMesh));
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
        ids_(std::move(ids_)),
        indices_(std::move(rhs.indices_)),
        rootTransform_(std::move(rhs.rootTransform_)),
        rootNode_(std::move(rhs.rootNode_)),
        materials_(std::move(rhs.materials_)),
        subMeshes_(std::move(rhs.subMeshes_)),
        nodes_(std::move(rhs.nodes_))
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
        ids_ = std::move(rhs.ids_);
        indices_ = std::move(rhs.indices_);
        rootTransform_ = std::move(rhs.rootTransform_);
        rootNode_ = std::move(rhs.rootNode_);
        materials_ = std::move(rhs.materials_);
        subMeshes_ = std::move(rhs.subMeshes_);
        nodes_ = std::move(rhs.nodes_);
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
        subMeshes_.push_back(std::make_unique<SubMesh>(this, name, idxOffset, numIndices, materialID));
    }

    /*void Mesh::CreateIndexBuffer()
    {
        iBuffer_ = std::make_unique<GLBuffer>(GL_STATIC_DRAW);
        OGL_CALL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, iBuffer_->GetBuffer());
        iBuffer_->InitializeData(indices_);
        OGL_CALL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
    }*/

    void MeshInfo::CreateSceneNodes(aiNode* rootNode)
    {
        rootNode_ = std::make_unique<SceneMeshNode>(rootNode, nullptr, subMeshes_);
        rootNode_->FlattenNodeTree(nodes_);
    }

    /*void Mesh::write(std::ofstream& ofs) const
    {
        VersionableSerializerType::writeHeader(ofs);
        serializeHelper::writeV(ofs, vertices_);
        serializeHelper::writeV(ofs, normals_);
        serializeHelper::writeVV(ofs, texCoords_);
        serializeHelper::writeV(ofs, tangents_);
        serializeHelper::writeV(ofs, binormals_);
        serializeHelper::writeVV(ofs, colors_);
        serializeHelper::writeVV(ofs, ids_);
        serializeHelper::writeV(ofs, indices_);

        serializeHelper::write(ofs, static_cast<uint64_t>(materials_.size()));
        for (const auto& mat : materials_) {
            serializeHelper::write(ofs, reinterpret_cast<uint64_t>(mat.get()));
            serializeHelper::write(ofs, mat->params.diffuseAlbedo);
            serializeHelper::write(ofs, mat->params.refraction);
            serializeHelper::write(ofs, mat->params.specularScaling);
            serializeHelper::write(ofs, mat->params.roughness);
            serializeHelper::write(ofs, mat->params.specularExponent);
            serializeHelper::write(ofs, mat->ambient);
            serializeHelper::write(ofs, mat->alpha);
            serializeHelper::write(ofs, mat->minOrientedAlpha);
            serializeHelper::write(ofs, mat->bumpMultiplier);
            if (mat->diffuseTex) serializeHelper::write(ofs, mat->diffuseTex->getId());
            else serializeHelper::write(ofs, std::string());

            if (mat->bumpTex) serializeHelper::write(ofs, mat->bumpTex->getId());
            else serializeHelper::write(ofs, std::string());
        }

        serializeHelper::write(ofs, static_cast<uint64_t>(subMeshes_.size()));
        for (const auto& mesh : subMeshes_) mesh->write(ofs);

        serializeHelper::write(ofs, rootTransform_);
        rootNode_->write(ofs);
    }

    bool Mesh::read(std::ifstream& ifs, TextureManager& texMan)
    {
        bool correctHeader;
        unsigned int actualVersion;
        std::tie(correctHeader, actualVersion) = VersionableSerializerType::checkHeader(ifs);
        if (correctHeader) {
            serializeHelper::readV(ifs, vertices_);
            serializeHelper::readV(ifs, normals_);
            serializeHelper::readVV(ifs, texCoords_);
            serializeHelper::readV(ifs, tangents_);
            serializeHelper::readV(ifs, binormals_);
            serializeHelper::readVV(ifs, colors_);
            serializeHelper::readVV(ifs, ids_);
            serializeHelper::readV(ifs, indices_);

            uint64_t numMaterials;
            std::unordered_map<uint64_t, Material*> materialMap;
            std::unordered_map<uint64_t, SubMesh*> meshMap;
            std::unordered_map<uint64_t, SceneMeshNode*> nodeMap;
            serializeHelper::read(ifs, numMaterials);
            materials_.resize(numMaterials);
            for (auto& mat : materials_) {
                mat.reset(new Material());
                uint64_t materialID;
                serializeHelper::read(ifs, materialID);
                serializeHelper::read(ifs, mat->params.diffuseAlbedo);
                serializeHelper::read(ifs, mat->params.refraction);
                serializeHelper::read(ifs, mat->params.specularScaling);
                serializeHelper::read(ifs, mat->params.roughness);
                serializeHelper::read(ifs, mat->params.specularExponent);
                serializeHelper::read(ifs, mat->ambient);
                serializeHelper::read(ifs, mat->alpha);
                serializeHelper::read(ifs, mat->minOrientedAlpha);
                serializeHelper::read(ifs, mat->bumpMultiplier);
                std::string diffuseTexId, bumpTexId;
                serializeHelper::read(ifs, diffuseTexId);
                serializeHelper::read(ifs, bumpTexId);
                if (diffuseTexId.size() > 0) mat->diffuseTex = texMan.GetResource(diffuseTexId);
                if (bumpTexId.size() > 0) mat->bumpTex = texMan.GetResource(bumpTexId);
                materialMap[materialID] = mat.get();
            }

            uint64_t numMeshes;

            serializeHelper::read(ifs, numMeshes);
            subMeshes_.resize(numMeshes);
            for (auto& mesh : subMeshes_) {
                mesh.reset(new SubMesh());
                if (!mesh->read(ifs, meshMap, materialMap)) return false;
            }

            serializeHelper::read(ifs, rootTransform_);
            rootNode_ = std::make_unique<SceneMeshNode>();
            nodeMap[0] = nullptr;
            if (!rootNode_->read(ifs, meshMap, nodeMap)) return false;
            return true;
        }
        return false;
    }*/
}
