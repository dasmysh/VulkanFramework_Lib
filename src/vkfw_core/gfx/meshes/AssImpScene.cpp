/**
 * @file   AssimpScene.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.02.16
 *
 * @brief  Contains the implementation of a scene loaded by AssImp.
 */

#include "gfx/meshes/AssimpScene.h"
#include "app/ApplicationBase.h"
#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/archives/binary.hpp>
#include "assimp_convert_helpers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(vku::gfx::AssImpScene, cereal::specialization::member_serialize)

namespace vku::gfx {

    inline glm::vec3 GetMaterialColor(aiMaterial* material, const char* pKey, unsigned int type, unsigned int idx) {
        aiColor3D c;
        material->Get(pKey, type, idx, c);
        return glm::vec3{ c.r, c.g, c.b };
    }

    AssImpScene::AssImpScene(const std::string& resourceId, const LogicalDevice* device, MeshCreateFlags flags)
        : Resource{resourceId, device}, meshFilename_{resourceId}
    {
        auto filename = FindResourceLocation(meshFilename_);

        if (!loadBinary(filename)) {
            createNewMesh(filename, flags);
            saveBinary(filename);
        }

        FlattenHierarchies();
    }

    /** Default copy constructor. */
    AssImpScene::AssImpScene(const AssImpScene& rhs) : Resource(rhs), MeshInfo(rhs), meshFilename_{rhs.meshFilename_} {}

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
    AssImpScene::AssImpScene(AssImpScene&& rhs) noexcept
        : Resource(std::move(rhs)), MeshInfo(std::move(rhs)), meshFilename_{std::move(rhs.meshFilename_)}
    {
    }

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

    void AssImpScene::createNewMesh(const std::string& filename, MeshCreateFlags flags)
    {
        GetBoneOffsetMatrixIndices().clear();
        GetBoneWeigths().clear();
        GetIndexVectors().clear();
        GetInverseBindPoseMatrices().clear();
        GetBoneBoundingBoxes().clear();
        GetSubMeshes().clear();
        GetAnimations().clear();

        unsigned int assimpFlags = (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs)
                                   & !aiProcess_CalcTangentSpace & !aiProcess_GenNormals & !aiProcess_GenSmoothNormals;
        if (flags & MeshCreateFlagBits::CREATE_TANGENTSPACE) assimpFlags |= aiProcess_CalcTangentSpace;
        if (flags & MeshCreateFlagBits::NO_SMOOTH_NORMALS) assimpFlags |= aiProcess_GenNormals;
        else assimpFlags |= aiProcess_GenSmoothNormals;

        Assimp::Importer importer;
        auto scene = importer.ReadFile(filename, assimpFlags);

        unsigned int maxUVChannels = 0, maxColorChannels = 0, numVertices = 0, numIndices = 0;
        bool hasTangentSpace = false;
        std::vector<std::vector<unsigned int>> indices;
        indices.resize(static_cast<size_t>(scene->mNumMeshes));
        auto numMeshes = static_cast<std::size_t>(scene->mNumMeshes);
        for (std::size_t i = 0; i < numMeshes; ++i) {
            maxUVChannels = glm::max(maxUVChannels, scene->mMeshes[i]->GetNumUVChannels());
            if (scene->mMeshes[i]->HasTangentsAndBitangents()) hasTangentSpace = true;
            maxColorChannels = glm::max(maxColorChannels, scene->mMeshes[i]->GetNumColorChannels());
            numVertices += scene->mMeshes[i]->mNumVertices;
            auto numFaces = static_cast<std::size_t>(scene->mMeshes[i]->mNumFaces);
            for (std::size_t fi = 0; fi < numFaces; ++fi) {
                auto faceIndices = scene->mMeshes[i]->mFaces[fi].mNumIndices;
                // TODO: currently lines and points are ignored. [12/14/2016 Sebastian Maisch]
                if (faceIndices == 3) {
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[0]);
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[1]);
                    indices[i].push_back(scene->mMeshes[i]->mFaces[fi].mIndices[2]);
                    numIndices += faceIndices;
                }
            }
        }

        std::filesystem::path sceneFilePath{ meshFilename_ };

        ReserveMesh(maxUVChannels, maxColorChannels, hasTangentSpace, numVertices, numIndices, scene->mNumMaterials);
        auto numMaterials = static_cast<std::size_t>(scene->mNumMaterials);
        for (std::size_t i = 0; i < numMaterials; ++i) {
            auto material = scene->mMaterials[i];
            auto mat = GetMaterial(i);
            mat->ambient_ = GetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT);
            mat->diffuse_ = GetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE);
            mat->specular_ = GetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR);
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

            if (material->GetTextureCount(aiTextureType_OPACITY) > 0) {
                mat->hasAlpha_ = true;
            }
        }

        unsigned int currentMeshIndexOffset = 0;
        unsigned int currentMeshVertexOffset = 0;
        std::map<std::string, unsigned int> bones;
        std::vector<std::vector<std::pair<unsigned int, float>>> boneWeights;
        boneWeights.resize(numVertices);
        for (unsigned int iMesh = 0; iMesh < scene->mNumMeshes; ++iMesh) {
            auto mesh = scene->mMeshes[iMesh];

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

            if (mesh->HasBones()) {

                // Walk all bones of this mesh
                for (auto b = 0U; b < mesh->mNumBones; ++b) {
                    auto aiBone = mesh->mBones[b];

                    auto bone = bones.find(aiBone->mName.C_Str());

                    // We don't have this bone, yet -> insert into mesh datastructure
                    if (bone == bones.end()) {
                        bones[aiBone->mName.C_Str()] = static_cast<unsigned int>(GetInverseBindPoseMatrices().size());

                        GetInverseBindPoseMatrices().push_back(AiMatrixToGLM(aiBone->mOffsetMatrix));
                    }

                    unsigned int indexOfCurrentBone = bones[aiBone->mName.C_Str()];

                    for (auto w = 0U; w < aiBone->mNumWeights; ++w) {

                        boneWeights[currentMeshVertexOffset + aiBone->mWeights[w].mVertexId].emplace_back(
                            indexOfCurrentBone, aiBone->mWeights[w].mWeight);
                    }
                }
            }
            else {
                for (std::size_t iVert = 0; iVert < mesh->mNumVertices; ++iVert) {
                    boneWeights[currentMeshVertexOffset + iVert].emplace_back(std::make_pair(0, 0.0f));
                }
            }

            if (!indices[iMesh].empty()) {
                std::transform(indices[iMesh].begin(), indices[iMesh].end(), &GetIndices()[currentMeshIndexOffset],
                    [currentMeshVertexOffset](unsigned int idx) { return static_cast<unsigned int>(idx + currentMeshVertexOffset); });
            }

            AddSubMesh(mesh->mName.C_Str(), currentMeshIndexOffset, static_cast<unsigned int>(indices[iMesh].size()), mesh->mMaterialIndex);
            currentMeshVertexOffset += mesh->mNumVertices;
            currentMeshIndexOffset += static_cast<unsigned int>(indices[iMesh].size());
        }

        // Parse parent information for each bone.
        GetBoneParents().resize(bones.size(), std::numeric_limits<std::size_t>::max());
        // Root node has a parent index of max value of size_t
        ParseBoneHierarchy(bones, scene->mRootNode, std::numeric_limits<std::size_t>::max(), glm::mat4(1.0f));

        // Iterate all weights for each vertex
        for (auto& weights : boneWeights) {
            // sort the weights.
            std::sort(weights.begin(), weights.end(),
                [](const std::pair<unsigned int, float>& left, const std::pair<unsigned int, float>& right) {
                return left.second > right.second;
            });

            // resize the weights, because we only take 4 bones per vertex into account.
            weights.resize(4);

            // build vec's with up to 4 components, one for each bone, influencing
            // the current vertex.
            glm::uvec4 newIndices;
            glm::vec4 newWeights;
            float sumWeights = 0.0f;
            for (auto i = 0U; i < weights.size(); ++i) {
                newIndices[i] = weights[i].first;
                newWeights[i] = weights[i].second;
                sumWeights += newWeights[i];
            }

            GetBoneOffsetMatrixIndices().push_back(newIndices);
            // normalize the bone weights.
            GetBoneWeigths().push_back(newWeights / glm::max(sumWeights, 0.000000001f));
        }

        // Loading animations
        if (scene->HasAnimations()) {
            for (auto a = 0U; a < scene->mNumAnimations; ++a) {
                GetAnimations().emplace_back(scene->mAnimations[a]);
            }
        }

        CreateSceneNodes(scene->mRootNode, bones);
    }

    void AssImpScene::saveBinary(const std::string& filename) const
    {
        BinaryOAWrapper oa{ filename };
        // oa(cereal::make_nvp("assimpMesh", *this));
        oa(cereal::base_class<MeshInfo>(this), cereal::make_nvp("meshFilename", meshFilename_));
    }

    bool AssImpScene::loadBinary(const std::string& filename)
    {
        try {
            BinaryIAWrapper ia{ filename };
            if (ia.IsValid()) {
                // ia(cereal::make_nvp("assimpMesh", *this));
                ia(cereal::base_class<MeshInfo>(this), cereal::make_nvp("meshFilename", meshFilename_));
                return true;
            }
        } catch (cereal::Exception e) {
            spdlog::error("Could not load binary file. Falling back to Assimp.\nResourceID: {}\nFilename: "
                          "{}\nDescription: Cereal Error.\nError Message: {}",
                          getId(), BinaryIAWrapper::GetBinFilename(filename), e.what());
            return false;
        } catch (...) {
            spdlog::error("Could not load binary file. Falling back to Assimp.\nResourceID: {}\nFilename: "
                          "{}\nDescription: Reason unknown",
                          getId(), BinaryIAWrapper::GetBinFilename(filename));
            return false;
        }
        return false;
    }

    ///
    /// This function walks the hierarchy of bones and does two things:
    /// - set the parent of each bone into `boneParent_`
    /// - update the boneOffsetMatrices_, so each matrix also includes the
    ///   transformations of the child bones.
    ///
    /// \param map from name of bone to index in boneOffsetMatrices_
    /// \param current node in
    /// \param index of the parent in boneOffsetMatrices_
    /// \param Matrix including all transformations from the parents of the
    ///        current node.
    ///
    void AssImpScene::ParseBoneHierarchy(const std::map<std::string, unsigned int>& bones, const aiNode* node,
        std::size_t parent, glm::mat4 parentMatrix)
    {
        auto bone = bones.find(node->mName.C_Str());
        if (bone != bones.end()) {
            // This node is a bone. Set the parent for this node to the current parent
            // node.
            GetBoneParents()[bone->second] = parent;
            // Set the new parent
            parent = bone->second;
        }

        for (auto i = 0U; i < node->mNumChildren; ++i) {
            ParseBoneHierarchy(bones, node->mChildren[i], parent, parentMatrix);
        }
    }
}
