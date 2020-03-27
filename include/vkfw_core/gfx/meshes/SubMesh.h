/**
 * @file   SubMesh.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Declaration of a sub mesh helper class.
 */

#pragma once

#include "main.h"
#include "core/math/primitives.h"
#include <cereal/cereal.hpp>

namespace vku::gfx {

    struct Material;
    class MeshInfo;

    /**
     * A SubMesh is a sub group of geometry in a mesh. It does not have its own
     * vertex information but uses indices to define which vertices of the mesh are used.
     */
    class SubMesh
    {
    public:
        SubMesh() = default;
        SubMesh(const MeshInfo* mesh, std::string objectName, unsigned int indexOffset, unsigned int numIndices, unsigned int materialID);
        SubMesh(const SubMesh&);
        SubMesh& operator=(const SubMesh&);
        SubMesh(SubMesh&&) noexcept;
        SubMesh& operator=(SubMesh&&) noexcept;
        virtual ~SubMesh();

        [[nodiscard]] const std::string& GetName() const { return objectName_; }
        [[nodiscard]] std::uint64_t GetSerializationID() const { return serializationID_; }
        [[nodiscard]] unsigned int GetIndexOffset() const { return indexOffset_; }
        [[nodiscard]] unsigned int GetNumberOfIndices() const { return numIndices_; }
        [[nodiscard]] unsigned int GetNumberOfTriangles() const { return numIndices_ / 3; }
        [[nodiscard]] const math::AABB3<float>& GetLocalAABB() const { return aabb_; }
        [[nodiscard]] unsigned int GetMaterialID() const { return materialID_; }

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template<class Archive> void serialize(Archive& ar, const std::uint32_t) // NOLINT
        {
            serializationID_ = reinterpret_cast<std::uint64_t>(this); // NOLINT
            ar(cereal::make_nvp("objectName", objectName_),
                cereal::make_nvp("serializationID", serializationID_),
                cereal::make_nvp("indexOffset", indexOffset_),
                cereal::make_nvp("numIndices", numIndices_),
                cereal::make_nvp("AABB", aabb_),
                cereal::make_nvp("material", materialID_));
        }

        /** Holds the sub-meshes object name. */
        std::string objectName_;
        /** Holds a serialization id. */
        std::uint64_t serializationID_ = 0;
        /** The index offset the sub-mesh starts. */
        unsigned int indexOffset_ = 0;
        /** The number of indices in the sub-mesh. */
        unsigned int numIndices_ = 0;
        /** The sub-meshes local AABB. */
        math::AABB3<float> aabb_;
        /** The sub-meshes material id. */
        unsigned int materialID_ = std::numeric_limits<unsigned int>::max();
    };
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vku::gfx::SubMesh, 1)
