/**
 * @file   ComponentFamilyManager.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  This Manager holds the different types of ComponentManagers in an
 * array and dispatches requests to this ComponentManagers using there
 * family attribute.
 */

#include "ComponentFamilyManager.h"

namespace vku {

    ComponentFamilyManager::ComponentFamilyManager(ComponentFamilyManager&& rhs) noexcept :
        componentManagers_{ std::move(rhs.componentManagers_) }
    {
    }

    ComponentFamilyManager& ComponentFamilyManager::operator=(ComponentFamilyManager&& rhs) noexcept
    {
        this->~ComponentFamilyManager();
        componentManagers_ = std::move(componentManagers_);
        return *this;
    }

    void ComponentFamilyManager::RemoveComponent(std::uint32_t family, std::uint32_t componentHandle)
    {
        auto componentManager = componentManagers_[family].get();

        componentManager->RemoveComponent(componentHandle);
    }

    std::uint32_t ComponentFamilyManager::CopyComponent(std::uint32_t family, std::uint32_t componentHandle)
    {
        auto componentManager = componentManagers_[family].get();
        return componentManager->CopyComponent(componentHandle);
    }
}
