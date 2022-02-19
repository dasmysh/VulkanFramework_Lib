/**
 * @file   AssImpScene.h
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

namespace vkfw_core::gfx {

    enum class MeshCreateFlagBits : unsigned int {
        NO_SMOOTH_NORMALS = 0x1,
        CREATE_TANGENTSPACE = 0x2
    };
}

namespace vkfw_core {
    template<> struct EnableBitMaskOperators<gfx::MeshCreateFlagBits> { static constexpr bool enable = true; };
}

namespace vkfw_core::gfx {

    using MeshCreateFlags = EnumFlags<MeshCreateFlagBits>;

    /**
     * @brief  Resource implementation for .obj files.
     * This is used to generate renderable meshes from .obj files.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2014.01.08
     */
    class AssImpScene : public Resource, public MeshInfo
    {
    public:
        AssImpScene(std::string resourceId, const LogicalDevice* device,
                    MeshCreateFlags flags = MeshCreateFlags());
        AssImpScene(const AssImpScene&);
        AssImpScene& operator=(const AssImpScene&);
        AssImpScene(AssImpScene&&) noexcept;
        AssImpScene& operator=(AssImpScene&&) noexcept;
        ~AssImpScene() override;

    private:
        void createNewMesh(const std::string& filename, MeshCreateFlags flags);
        void ParseBoneHierarchy(const std::map<std::string, unsigned int>& bones, const aiNode* node,
            std::size_t parent, glm::mat4 parentMatrix);

        void saveBinary(const std::string& filename) const;
        bool loadBinary(const std::string& filename);

        /** Holds the meshes filename */
        std::string m_meshFilename;
    };
}
