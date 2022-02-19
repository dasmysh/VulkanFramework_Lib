/**
 * @file   Material.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.24
 *
 * @brief  Implementation of the material class.
 */

#include "gfx/Material.h"
#include "gfx/Texture2D.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/textures/DeviceTexture.h"

namespace vkfw_core::gfx {

    inline std::size_t NoMaterialInfo::GetGPUSize() { return sizeof(materials::NoMaterial); }

    void NoMaterialInfo::FillGPUInfo([[maybe_unused]] const NoMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                     [[maybe_unused]] std::uint32_t firstTextureIndex)
    {
        auto mat = reinterpret_cast<materials::NoMaterial*>(gpuInfo.data());
        mat->dummy = 0xDEADBEEF;
    }

    std::unique_ptr<MaterialInfo> NoMaterialInfo::copy() { return std::make_unique<NoMaterialInfo>(*this); }

    std::size_t PhongMaterialInfo::GetGPUSize() { return sizeof(materials::PhongMaterial); }

    void PhongMaterialInfo::FillGPUInfo(const PhongMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                            std::uint32_t firstTextureIndex)
    {
        auto mat = reinterpret_cast<materials::PhongMaterial*>(gpuInfo.data());
        mat->ambient = info.m_ambient;
        mat->diffuse = info.m_diffuse;
        mat->specular = info.m_specular;
        mat->alpha = info.m_alpha;
        mat->specularExponent = info.m_specularExponent;
        mat->diffuseTextureIndex = firstTextureIndex;
    }

    std::unique_ptr<MaterialInfo> PhongMaterialInfo::copy() { return std::make_unique<PhongMaterialInfo>(*this); }

    std::size_t PhongBumpMaterialInfo::GetGPUSize() { return sizeof(materials::PhongBumpMaterial); }

    void PhongBumpMaterialInfo::FillGPUInfo(const PhongBumpMaterialInfo& info, std::span<std::uint8_t>& gpuInfo,
                                            std::uint32_t firstTextureIndex)
    {
        auto mat = reinterpret_cast<materials::PhongBumpMaterial*>(gpuInfo.data());
        mat->ambient = info.m_ambient;
        mat->diffuse = info.m_diffuse;
        mat->specular = info.m_specular;
        mat->alpha = info.m_alpha;
        mat->specularExponent = info.m_specularExponent;
        mat->bumpMultiplier = info.m_bumpMultiplier;
        mat->diffuseTextureIndex = firstTextureIndex;
        mat->bumpTextureIndex = firstTextureIndex + 1;
    }

    std::unique_ptr<MaterialInfo> PhongBumpMaterialInfo::copy()
    {
        return std::make_unique<PhongBumpMaterialInfo>(*this);
    }

    Material::Material() :
        m_materialInfo{ nullptr }
    {
    }

    Material::Material(const MaterialInfo* materialInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        m_materialInfo{ materialInfo }
    {
        assert(m_materialInfo != nullptr);
        auto textureCount = m_materialInfo->GetTextureCount();
        m_textures.resize(textureCount);
        for (std::size_t i = 0; i < textureCount; ++i) {
            auto textureName = m_materialInfo->GetTextureFilename(i);
            if (!textureName.empty()) {
                m_textures[i] =
                    device->GetTextureManager()->GetResource(textureName, true, true, memoryGroup, queueFamilyIndices);
            }
        }
    }

    Material::Material(const Material& rhs) = default;
    Material& Material::operator=(const Material& rhs) = default;

    Material::Material(Material&& rhs) noexcept :
        m_materialInfo{ rhs.m_materialInfo },
        m_textures{ std::move(rhs.m_textures) }
    {
    }

    Material& Material::operator=(Material&& rhs) noexcept
    {
        this->~Material();
        m_materialInfo = rhs.m_materialInfo;
        m_textures = std::move(rhs.m_textures);
        return *this;
    }

    Material::~Material() = default;

    void Material::CreateResourceUseBarriers(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStage,
                                             vk::ImageLayout newLayout, PipelineBarrier& barrier)
    {
        for (auto& texture : m_textures) {
            if (texture) { texture->GetTexture().AccessBarrier(access, pipelineStage, newLayout, barrier); }
        }
    }
}
