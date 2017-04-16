/**
 * @file   AssimpScene.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.02.16
 *
 * @brief  Contains the implementation of a scene loaded by AssImp.
 */

#define GLM_FORDE_SWIZZLE
#include "AssimpScene.h"
#include "app/ApplicationBase.h"
#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>
#include <cereal/archives/binary.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace vku::gfx {

    AssImpScene::AssImpScene(const std::string& resourceId, const LogicalDevice* device,
        const std::string& meshFilename, MeshCreateFlags flags) :
        Resource{ resourceId, device },
        meshFilename_{ meshFilename }
    {
        auto filename = FindResourceLocation(meshFilename_);
        auto binFilename = filename + ".myshbin";

        if (!load(binFilename)) createNewMesh(filename, binFilename, flags);

        /*CreateIndexBuffer();
        auto rootScale = GetNamedParameterValue("scale", 1.0f);
        auto rootScaleV = GetNamedParameterValue("scaleV", glm::vec3(1.0f));
        auto rootTranslate = GetNamedParameterValue("translate", glm::vec3(0.0f));
        auto matScale = glm::scale(rootScaleV * rootScale);
        auto matTranslate = glm::translate(rootTranslate);
        SetRootTransform(matTranslate * matScale);*/
    }

    /**
     * Constructor.
     * @param meshFilename the mesh files file name.
     * @param mtlLibManager the material library manager to use
     */
    AssImpScene::AssImpScene(const std::string& meshFilename, const LogicalDevice* device, MeshCreateFlags flags) :
        AssImpScene(meshFilename, device, meshFilename, flags)
    {
    }

    /** Default copy constructor. */
    AssImpScene::AssImpScene(const AssImpScene& rhs) : Resource(rhs), MeshInfo(rhs) {}

    /** Default copy assignment operator. */
    AssImpScene& AssImpScene::operator=(const AssImpScene& rhs)
    {
        if (this != &rhs) {
            AssImpScene tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    /** Default move constructor. */
    AssImpScene::AssImpScene(AssImpScene&& rhs) noexcept : Resource(std::move(rhs)), MeshInfo(std::move(rhs)) {}

    /** Default move assignment operator. */
    AssImpScene& AssImpScene::operator=(AssImpScene&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~AssImpScene();
            Resource* tRes = this;
            *tRes = static_cast<Resource&&>(std::move(rhs));
            MeshInfo* tMesh = this;
            *tMesh = static_cast<MeshInfo&&>(std::move(rhs));
        }
        return *this;
    }

    /** Destructor. */
    AssImpScene::~AssImpScene() = default;

    void AssImpScene::createNewMesh(const std::string& filename, const std::string& binFilename, MeshCreateFlags flags)
    {
        unsigned int assimpFlags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_LimitBoneWeights
            | aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes
            | aiProcess_OptimizeGraph;
        if (flags & MeshCreateFlagBits::CREATE_TANGENTSPACE) assimpFlags |= aiProcess_CalcTangentSpace;
        if (flags & MeshCreateFlagBits::NO_SMOOTH_NORMALS) assimpFlags |= aiProcess_GenNormals;
        else assimpFlags |= aiProcess_GenSmoothNormals;

        Assimp::Importer importer;
        auto scene = importer.ReadFile(filename, assimpFlags);

        unsigned int maxUVChannels = 0, maxColorChannels = 0, numVertices = 0, numIndices = 0;
        bool hasTangentSpace = false;
        std::vector<std::vector<unsigned int>> indices;
        indices.resize(scene->mNumMeshes);
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            maxUVChannels = glm::max(maxUVChannels, scene->mMeshes[i]->GetNumUVChannels());
            if (scene->mMeshes[i]->HasTangentsAndBitangents()) hasTangentSpace = true;
            maxColorChannels = glm::max(maxColorChannels, scene->mMeshes[i]->GetNumColorChannels());
            numVertices += scene->mMeshes[i]->mNumVertices;
            for (unsigned int fi = 0; fi < scene->mMeshes[i]->mNumFaces; ++fi) {
                auto faceIndices = scene->mMeshes[i]->mFaces[fi].mNumIndices;
                if (faceIndices == 3) {
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[0]);
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[1]);
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[2]);
                    numIndices += faceIndices;
                } else {
                    // TODO: handle points and lines. [2/17/2016 Sebastian Maisch]
                }
            }
        }

        std::experimental::filesystem::path sceneFilePath{ meshFilename_ };

        ReserveMesh(maxUVChannels, maxColorChannels, hasTangentSpace, numVertices, numIndices, scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            auto material = scene->mMaterials[i];
            auto mat = GetMaterial(i);
            material->Get(AI_MATKEY_COLOR_AMBIENT, mat->ambient_);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, mat->diffuse_);
            material->Get(AI_MATKEY_COLOR_SPECULAR, mat->specular_);
            material->Get(AI_MATKEY_OPACITY, mat->alpha_);
            material->Get(AI_MATKEY_SHININESS, mat->specularExponent_);
            material->Get(AI_MATKEY_REFRACTI, mat->refraction_);
            aiString materialName, diffuseTexPath, bumpTexPath;
            if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, materialName)) mat->materialName_ = materialName.C_Str();

            if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuseTexPath)) {
                mat->diffuseTextureFilename_ = sceneFilePath.parent_path().string() + "/" + diffuseTexPath.C_Str();
            }

            if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), bumpTexPath)) {
                mat->bumpMapFilename_ = sceneFilePath.parent_path().string() + "/" + bumpTexPath.C_Str();
                material->Get(AI_MATKEY_TEXBLEND(aiTextureType_HEIGHT, 0), mat->bumpMultiplier_);
            } else if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), bumpTexPath)) {
                mat->bumpMapFilename_ = sceneFilePath.parent_path().string() + "/" + bumpTexPath.C_Str();
                material->Get(AI_MATKEY_TEXBLEND(aiTextureType_NORMALS, 0), mat->bumpMultiplier_);
            }
        }

        unsigned int currentMeshIndexOffset = 0;
        unsigned int currentMeshVertexOffset = 0;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            auto mesh = scene->mMeshes[i];

            if (mesh->HasPositions()) {
                std::copy(mesh->mVertices, &mesh->mVertices[mesh->mNumVertices], reinterpret_cast<aiVector3D*>(&GetVertices()[currentMeshVertexOffset]));
            }
            if (mesh->HasNormals()) {
                std::copy(mesh->mNormals, &mesh->mNormals[mesh->mNumVertices], reinterpret_cast<aiVector3D*>(&GetNormals()[currentMeshVertexOffset]));
            }
            for (unsigned int ti = 0; ti < mesh->GetNumUVChannels(); ++ti) {
                std::copy(mesh->mTextureCoords[ti], &mesh->mTextureCoords[ti][mesh->mNumVertices], reinterpret_cast<aiVector3D*>(&GetTexCoords()[ti][currentMeshVertexOffset]));
            }
            if (mesh->HasTangentsAndBitangents()) {
                std::copy(mesh->mTangents, &mesh->mTangents[mesh->mNumVertices], reinterpret_cast<aiVector3D*>(&GetTangents()[currentMeshVertexOffset]));
                std::copy(mesh->mBitangents, &mesh->mBitangents[mesh->mNumVertices], reinterpret_cast<aiVector3D*>(&GetBinormals()[currentMeshVertexOffset]));
            }
            for (unsigned int ci = 0; ci < mesh->GetNumColorChannels(); ++ci) {
                std::copy(mesh->mColors[ci], &mesh->mColors[ci][mesh->mNumVertices], reinterpret_cast<aiColor4D*>(&GetColors()[ci][currentMeshVertexOffset]));
            }

            std::transform(indices[i].begin(), indices[i].end(), &GetIndices()[currentMeshIndexOffset], [currentMeshVertexOffset](unsigned int idx){ return idx + currentMeshVertexOffset; });

            AddSubMesh(mesh->mName.C_Str(), currentMeshIndexOffset, static_cast<unsigned int>(indices[i].size()), mesh->mMaterialIndex);
            currentMeshVertexOffset += mesh->mNumVertices;
            currentMeshIndexOffset += static_cast<unsigned int>(indices[i].size());
        }

        CreateSceneNodes(scene->mRootNode);
        save(binFilename);
    }

    /*std::string AssimpScene::GetFullFilename() const
    {
        return Resource::FindResourceLocation(Resource::GetFilename());
    }*/

    /*std::shared_ptr<const GLTexture2D> AssimpScene::loadTexture(const std::string& relFilename, const std::string& params, ApplicationBase* app) const
    {
        boost::filesystem::path sceneFilePath{ GetParameters()[0] };
        auto texFilename = sceneFilePath.parent_path().string() + "/" + relFilename + (params.size() > 0 ? "," + params : "");
        return std::move(app->GetTextureManager()->GetResource(texFilename));
    }*/

    void AssImpScene::save(const std::string& filename) const
    {
        std::ofstream ofs(filename, std::ios::out | std::ios::binary);
        cereal::BinaryOutputArchive oa(ofs);
        oa(cereal::make_nvp("assimpMesh", *this));
    }

    bool AssImpScene::load(const std::string& filename)
    {
        if (std::experimental::filesystem::exists(filename)) {
            std::ifstream inBinFile(filename, std::ios::binary);
            if (inBinFile.is_open()) {
                try {
                    cereal::BinaryInputArchive ia(inBinFile);
                    ia(cereal::make_nvp("assimpMesh", *this));
                    return true;
                }
                catch (cereal::Exception e) {
                    LOG(ERROR) << "Could not load binary file. Falling back to Assimp." << std::endl
                        << "ResourceID: " << getId() << std::endl
                        << "Filename: " << filename << std::endl
                        << "Description: Cereal Error." << std::endl
                        << "Error Message: " << e.what();
                    return false;
                }
                catch (...) {
                    LOG(ERROR) << "Could not load binary file. Falling back to AssImp." << std::endl
                        << "ResourceID: " << getId() << std::endl
                        << "Filename: " << filename << std::endl
                        << "Description: Reason unknown.";
                    return false;
                }
            } else LOG(ERROR) << "Could not load binary file. Falling back to AssImp." << std::endl
                << "ResourceID: " << getId() << std::endl
                << "Filename: " << filename << std::endl
                << "Description: Could not open file.";
        } else LOG(ERROR) << "Could not load binary file. Falling back to AssImp." << std::endl
            << "ResourceID: " << getId() << std::endl
            << "Filename: " << filename << std::endl
            << "Description: File does not exist.";
        return false;
    }
}
