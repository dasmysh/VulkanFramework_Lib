/**
 * @file   Renderable.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Renderable declaration.
 */

#pragma once

#include <functional>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/mat3x4.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <typeindex>

namespace vku::gfx {

    struct LocalTransform {
        LocalTransform(const glm::mat4& modelMatrix) :
            modelMatrix_{ modelMatrix }, normalMatrix_{ glm::inverseTranspose(glm::mat3(modelMatrix)) } {}
        /** Holds the model matrix. */
        glm::mat4 modelMatrix_;
        /** Holds the normal matrix. */
        glm::mat3x4 normalMatrix_;
    };

    struct MaterialInfo;
    class Mesh;

    class Renderable {
    public:
        Renderable(bool has_indices);
        Renderable(const Renderable&);
        Renderable& operator=(const Renderable&);
        Renderable(Renderable&&) noexcept;
        Renderable& operator=(Renderable&&) noexcept;
        virtual ~Renderable();

        bool HasIndices() const { return has_indices_; };

        virtual std::size_t GetNumberOfNodes() const;
        virtual std::size_t GetNumberOfMaterials() const;
        virtual std::size_t GetNumberOfPartsInNode(std::size_t node_id) const;
        virtual std::size_t GetTotalElementCount() const;
        virtual std::size_t GetTotalVertexCount() const;
        virtual std::size_t GetIndices(std::vector<std::uint32_t>& indices) const;
        virtual std::size_t GetTotalVertexCount() const;

        virtual std::size_t FillLocalTransforms(std::vector<std::uint8_t>& localTransforms, const glm::mat4& modelMatrix, std::size_t alignment) const;
        virtual std::size_t UpdateLocalTransforms(std::vector<std::uint8_t>& localTransforms, const glm::mat4& modelMatrix, std::size_t alignment) const;
        virtual glm::mat4 GetLocalTransform(std::size_t node_id) const;

        virtual std::size_t GetObjectPartID(std::size_t nodeID, std::size_t partID) const = 0;
        virtual std::uint32_t GetElementCount(std::size_t objectPartID) const = 0;
        virtual std::uint32_t GetFirstElement(std::size_t objectPartID) const = 0;
        virtual const MaterialInfo& GetMaterial(std::size_t nodeID, std::size_t partID) const = 0;
        virtual const MaterialInfo& GetMaterial(std::size_t materialID) const = 0;

        // TODO: maybe we need a more general interface instead of MeshInfo. [4/17/2017 Sebastian Maisch]
        virtual const MeshInfo* GetMeshInfo() const;

    private:
        /// Has this renderable indices or just vertices?
        bool has_indices_;
    };

}
