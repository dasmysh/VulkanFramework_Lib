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

namespace vkfw_core::gfx {


    Material::Material() :
        m_materialInfo{ nullptr }
    {
    }

    Material::Material(const MaterialInfo* materialInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        m_materialInfo{ materialInfo }
    {
        assert(m_materialInfo != nullptr);
        if (!m_materialInfo->m_diffuseTextureFilename.empty()) {
            m_diffuseTexture = device->GetTextureManager()->GetResource(materialInfo->m_diffuseTextureFilename, true,
                                                                       true, memoryGroup, queueFamilyIndices);
        }
        if (!m_materialInfo->m_bumpMapFilename.empty()) {
            m_bumpMap = device->GetTextureManager()->GetResource(materialInfo->m_bumpMapFilename, true, true, memoryGroup,
                                                                queueFamilyIndices);
        }
    }

    Material::Material(const Material& rhs) = default;
    Material& Material::operator=(const Material& rhs) = default;

    Material::Material(Material&& rhs) noexcept :
        m_materialInfo{ rhs.m_materialInfo },
        m_diffuseTexture{ std::move(rhs.m_diffuseTexture) },
        m_bumpMap{ std::move(rhs.m_bumpMap) }
    {
    }

    Material& Material::operator=(Material&& rhs) noexcept
    {
        this->~Material();
        m_materialInfo = rhs.m_materialInfo;
        m_diffuseTexture = std::move(rhs.m_diffuseTexture);
        m_bumpMap = std::move(rhs.m_bumpMap);
        return *this;
    }

    Material::~Material() = default;
}
