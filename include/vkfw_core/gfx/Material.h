/**
 * @file   Material.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.14
 *
 * @brief  Declaration of the material class.
 */

#pragma once

#include "main.h"
#include "materials/material_host_interface.h"

#include <glm/vec3.hpp>

namespace vkfw_core::gfx {

    class Texture2D;
    class LogicalDevice;
    class MemoryGroup;
    class PipelineBarrier;

    struct MaterialInfo
    {
        MaterialInfo() = default;
        MaterialInfo(std::string_view name, std::uint32_t materialId)
            : m_materialName{name}, m_materialIdentifier{materialId}
        {
        }
        virtual ~MaterialInfo() = default;

        /** Holds the materials name. */
        std::string m_materialName;
        /** Holds whether this material has an alpha channel. */
        bool m_hasAlpha = false;
        /** The id of the material representation. */
        std::uint32_t m_materialIdentifier = static_cast<std::uint32_t>(-1);
        /** The textures in this material. */
        std::vector<std::string> m_textureFilenames;

        std::size_t GetTextureCount() const { return m_textureFilenames.size(); }
        const std::string& GetTextureFilename(std::size_t textureIndex) const { return m_textureFilenames[textureIndex]; }
        virtual std::unique_ptr<MaterialInfo> copy() = 0;

        template<class Archive> void serialize(Archive& ar, const std::uint32_t version) // NOLINT
        {
            ar(cereal::make_nvp("materialName", m_materialName), cereal::make_nvp("hasAlpha", m_hasAlpha),
               cereal::make_nvp("materialIdentifier", m_materialIdentifier),
               cereal::make_nvp("textureFilenames", m_textureFilenames));
        }
    };

    struct NoMaterialInfo : public MaterialInfo
    {
        static constexpr std::uint32_t MATERIAL_ID =
            static_cast<std::uint32_t>(materials::MaterialIdentifierCore::NoMaterialType);

        NoMaterialInfo() : MaterialInfo("NoMaterial", MATERIAL_ID) {}

        static std::size_t GetGPUSize();
        static void FillGPUInfo(const NoMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                std::uint32_t firstTextureIndex);
        std::unique_ptr<MaterialInfo> copy() override;

        template<class Archive> void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) // NOLINT
        {
            ar(cereal::base_class<MaterialInfo>(this));
        }
    };

    struct PhongMaterialInfo : public MaterialInfo
    {
        static constexpr std::uint32_t MATERIAL_ID =
            static_cast<std::uint32_t>(materials::MaterialIdentifierCore::PhongMaterialType);

        PhongMaterialInfo() : MaterialInfo("PhongMaterial", MATERIAL_ID) {}
        PhongMaterialInfo(std::string_view name) : MaterialInfo(name, MATERIAL_ID) {}

        /** Holds the materials ambient color. */
        glm::vec3 m_ambient = glm::vec3{1.0f};
        /** Holds the materials diffuse albedo. */
        glm::vec3 m_diffuse = glm::vec3{1.0f};
        /** Holds the materials specular albedo. */
        glm::vec3 m_specular = glm::vec3{1.0f};
        /** Holds the materials alpha value. */
        float m_alpha = 1.0f;
        /** Holds the materials specular exponent. */
        float m_specularExponent = 0.0f;

        static std::size_t GetGPUSize();
        static void FillGPUInfo(const PhongMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                std::uint32_t firstTextureIndex);
        std::unique_ptr<MaterialInfo> copy() override;

        template<class Archive> void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) // NOLINT
        {
            ar(cereal::base_class<MaterialInfo>(this),
               cereal::make_nvp("ambientColor", m_ambient), cereal::make_nvp("diffuseColor", m_diffuse),
               cereal::make_nvp("specularColor", m_specular), cereal::make_nvp("alpha", m_alpha),
               cereal::make_nvp("specularExponent", m_specularExponent));

        }

    protected:
        PhongMaterialInfo(std::string_view name, std::uint32_t materialId) : MaterialInfo(name, materialId) {}
    };

    struct PhongBumpMaterialInfo : public PhongMaterialInfo
    {
        static constexpr std::uint32_t MATERIAL_ID =
            static_cast<std::uint32_t>(materials::MaterialIdentifierCore::PhongBumpMaterialType);

        PhongBumpMaterialInfo() : PhongMaterialInfo("PhongBumpMaterial", MATERIAL_ID) {}
        PhongBumpMaterialInfo(std::string_view name) : PhongMaterialInfo(name, MATERIAL_ID) {}

        /** Holds the materials bump multiplier. */
        float m_bumpMultiplier = 1.0f;

        static std::size_t GetGPUSize();
        static void FillGPUInfo(const PhongBumpMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                std::uint32_t firstTextureIndex);
        std::unique_ptr<MaterialInfo> copy() override;

        template<class Archive> void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) // NOLINT
        {
            ar(cereal::base_class<PhongMaterialInfo>(this),
               cereal::make_nvp("bumpMultiplier", m_bumpMultiplier));
        }
    };

    struct Material final
    {
        Material();
        Material(const MaterialInfo* materialInfo, const LogicalDevice* device, MemoryGroup& memoryGroup,
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        Material(const Material&);
        Material& operator=(const Material&);
        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;
        ~Material();

        void CreateResourceUseBarriers(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStage,
                                       vk::ImageLayout newLayout, PipelineBarrier& barrier);

        /** Holds the material information. */
        const MaterialInfo* m_materialInfo = nullptr;
        /** Holds the materials textures. */
        std::vector<std::shared_ptr<Texture2D>> m_textures;
    };
}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::MaterialInfo, 3)
CEREAL_CLASS_VERSION(vkfw_core::gfx::NoMaterialInfo, 1)
CEREAL_CLASS_VERSION(vkfw_core::gfx::PhongMaterialInfo, 1)
CEREAL_CLASS_VERSION(vkfw_core::gfx::PhongBumpMaterialInfo, 1)
