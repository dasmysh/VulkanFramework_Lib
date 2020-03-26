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

namespace vku::gfx {


    Material::Material() :
        materialInfo_{ nullptr }
    {
    }

    Material::Material(const MaterialInfo* materialInfo, const LogicalDevice* device,
        MemoryGroup& memoryGroup, const std::vector<std::uint32_t>& queueFamilyIndices) :
        materialInfo_{ materialInfo }
    {
        assert(materialInfo_ != nullptr);
        if (!materialInfo_->diffuseTextureFilename_.empty())
            diffuseTexture_ = device->GetTextureManager()->GetResource(
                materialInfo->diffuseTextureFilename_, true, true, memoryGroup, queueFamilyIndices);
        if (!materialInfo_->bumpMapFilename_.empty())
            bumpMap_ = device->GetTextureManager()->GetResource(materialInfo->bumpMapFilename_, true, true, memoryGroup,
                                                                queueFamilyIndices);
    }

    Material::Material(const Material& rhs) :
        materialInfo_{ rhs.materialInfo_ },
        diffuseTexture_{ rhs.diffuseTexture_ },
        bumpMap_{ rhs.bumpMap_ }
    {
    }

    Material& Material::operator=(const Material& rhs)
    {
        if (this != &rhs) {
            Material tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    Material::Material(Material&& rhs) noexcept :
        materialInfo_{ rhs.materialInfo_ },
        diffuseTexture_{ std::move(rhs.diffuseTexture_) },
        bumpMap_{ std::move(rhs.bumpMap_) }
    {
    }

    Material& Material::operator=(Material&& rhs) noexcept
    {
        this->~Material();
        materialInfo_ = rhs.materialInfo_;
        diffuseTexture_ = std::move(rhs.diffuseTexture_);
        bumpMap_ = std::move(rhs.bumpMap_);
        return *this;
    }

    Material::~Material() = default;
}
