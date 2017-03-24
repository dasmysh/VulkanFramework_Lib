/**
 * @file   AssimpScene.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.02.16
 *
 * @brief  Contains the definition of a scene loaded by AssImp.
 */

#pragma once

#include "main.h"
#include "gfx/Material.h"
#include "MeshInfo.h"
#include <core/serialization_helper.h>
#include <core/enum_flags.h>
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/base_class.hpp>

namespace vku::gfx {

    enum class MeshCreateFlagBits {
        NO_SMOOTH_NORMALS = 0x1,
        CREATE_TANGENTSPACE = 0x2
    };
}

namespace vku {
    template<> struct EnableBitMaskOperators<gfx::MeshCreateFlagBits> { static constexpr bool enable = true; };
}

namespace vku::gfx {

    using MeshCreateFlags = EnumFlags<MeshCreateFlagBits>;

    /**
     * @brief  Resource implementation for .obj files.
     * This is used to generate renderable meshes from .obj files.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2014.01.08
     */
    class AssimpScene : public Resource, public MeshInfo
    {
    public:
        AssimpScene(const std::string& resourceId, const LogicalDevice* device, const std::string& meshFilename, MeshCreateFlags flags = MeshCreateFlags());
        AssimpScene(const std::string& meshFilename, const LogicalDevice* device, MeshCreateFlags flags = MeshCreateFlags());
        AssimpScene(const AssimpScene&);
        AssimpScene& operator=(const AssimpScene&);
        AssimpScene(AssimpScene&&) noexcept;
        AssimpScene& operator=(AssimpScene&&) noexcept;
        virtual ~AssimpScene();

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar, const std::uint32_t)
        {
            ar(cereal::base_class<Mesh>(this),
                cereal::make_nvp("meshFilename", meshFilename_));
        }

        void createNewMesh(const std::string& filename, const std::string& binFilename, MeshCreateFlags flags);
        void save(const std::string& filename) const;
        bool load(const std::string& filename);

        /** Holds the meshes filename */
        std::string meshFilename_;
    };
}

CEREAL_CLASS_VERSION(vku::gfx::AssimpScene, 1)
