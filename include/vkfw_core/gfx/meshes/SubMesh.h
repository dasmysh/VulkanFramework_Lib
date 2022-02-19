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

namespace vkfw_core::gfx {

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

        [[nodiscard]] const std::string& GetName() const { return m_objectName; }
        [[nodiscard]] std::uint64_t GetSerializationID() const { return m_serializationID; }
        [[nodiscard]] unsigned int GetIndexOffset() const { return m_indexOffset; }
        [[nodiscard]] unsigned int GetNumberOfIndices() const { return m_numIndices; }
        [[nodiscard]] unsigned int GetNumberOfTriangles() const { return m_numIndices / 3; }
        [[nodiscard]] const math::AABB3<float>& GetLocalAABB() const { return m_aabb; }
        [[nodiscard]] unsigned int GetMaterialID() const { return m_materialID; }

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template<class Archive> void serialize(Archive& ar, const std::uint32_t) // NOLINT
        {
            m_serializationID = reinterpret_cast<std::uint64_t>(this); // NOLINT
            ar(cereal::make_nvp("objectName", m_objectName),
                cereal::make_nvp("serializationID", m_serializationID),
                cereal::make_nvp("indexOffset", m_indexOffset),
                cereal::make_nvp("numIndices", m_numIndices),
                cereal::make_nvp("AABB", m_aabb),
                cereal::make_nvp("material", m_materialID));
        }

        /** Holds the sub-meshes object name. */
        std::string m_objectName;
        /** Holds a serialization id. */
        std::uint64_t m_serializationID = 0;
        /** The index offset the sub-mesh starts. */
        unsigned int m_indexOffset = 0;
        /** The number of indices in the sub-mesh. */
        unsigned int m_numIndices = 0;
        /** The sub-meshes local AABB. */
        math::AABB3<float> m_aabb;
        /** The sub-meshes material id. */
        unsigned int m_materialID = std::numeric_limits<unsigned int>::max();
    };
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::SubMesh, 1)
