/**
 * @file   ComponentFamilyManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  This Manager holds the different families of ComponentManagers in an
 * array and dispatches requests to this ComponentManagers using their
 * family attribute.
 */

#pragma once

#include "Component.h"
#include "ComponentManager.h"

#include <array>
#include <memory>

namespace vku {

    class ComponentFamilyManager
    {
    public:
        ComponentFamilyManager() noexcept = default;
        ComponentFamilyManager(const ComponentFamilyManager&) = delete;
        ComponentFamilyManager& operator=(const ComponentFamilyManager&) = delete;
        ComponentFamilyManager(ComponentFamilyManager&&) noexcept;
        ComponentFamilyManager& operator=(ComponentFamilyManager&&) noexcept;

        template<typename T, typename... Args> std::uint32_t SetComponent(std::uint32_t, Args&&...);
        template<typename T, typename... Args> void UpdateComponent(std::uint32_t, std::uint32_t, Args&&...);
        template<typename T> void RemoveComponent(std::uint32_t);
        void RemoveComponent(std::uint32_t, std::uint32_t);
        std::uint32_t CopyComponent(std::uint32_t, std::uint32_t);
        template<typename T> T* GetComponent(std::uint32_t);
        template<typename T> ComponentManager<T>* GetComponentMangerForFamily() const;

    private:
        template<typename T> void EnsureComponentManager();

        std::array<std::unique_ptr<BaseComponentManager>, MAX_COMPONENT_FAMILIES>
            componentManagers_;
    };

    ///
    /// Creates a new Component of type T for the given GameObject.
    /// \param Handle to the GameObject
    /// \param Arguments for the new Component
    ///
    template<typename T, typename... Args>
    std::uint32_t ComponentFamilyManager::SetComponent(std::uint32_t sceneObjectHandle,
        Args&&... args)
    {
        // Make sure there is a ComponentManger for the type T
        EnsureComponentManager<T>();

        std::uint32_t family = T::Family();
        // TODO maybe we can get rid of this cast...
        ComponentManager<T>* componentManager =
            reinterpret_cast<ComponentManager<T>*>(componentManagers_[family].get());

        // Creates a new Component and returns a handle to the component
        return componentManager->SetComponent(sceneObjectHandle,
            std::forward<Args>(args)...);
    }

    ///
    /// Updates a Component of type T for the given GameObject at a given position.
    /// \param Handle to the GameObject
    /// \param Handle to the Component to replace.
    /// \param Arguments for the new Component
    ///
    template<typename T, typename... Args>
    void ComponentFamilyManager::UpdateComponent(std::uint32_t sceneObjectHandle,
        std::uint32_t componentHandle, Args&&... args)
    {
        std::uint32_t family = T::Family();
        // TODO maybe we can get rid of this cast...
        ComponentManager<T>* componentManager =
            reinterpret_cast<ComponentManager<T>*>(componentManagers_[family].get());

        componentManager->UpdateComponent(sceneObjectHandle, componentHandle,
            std::forward<Args>(args)...);
    }

    ///
    /// Deletes a Component of type T.
    /// \param Handle to Component to delete
    ///
    template<typename T>
    void ComponentFamilyManager::RemoveComponent(std::uint32_t componentHandle)
    {
        std::uint32_t family = T::Family();
        RemoveComponent(family, componentHandle);
    }

    ///
    /// Gets the Component which the given Handle is pointing to.
    /// \param Handle to the Component
    /// \return Pointer to the Component
    ///
    template<typename T>
    T* ComponentFamilyManager::GetComponent(std::uint32_t componentHandle)
    {
        std::uint32_t family = T::Family();
        if (!componentManagers_[family]) {
            // there is no ComponentManager for type T
            return nullptr;
        }

        // TODO maybe we can get rid of this cast...
        ComponentManager<T>* componentManager =
            reinterpret_cast<ComponentManager<T>*>(componentManagers_[family].get());

        // TODO don't look at this line... 🙈
        return &(*componentManager)[componentHandle];
    }

    ///
    /// Ensure there is a ComponentManager to type T.
    ///
    template<typename T> void ComponentFamilyManager::EnsureComponentManager()
    {
        std::uint32_t family = T::Family();
        if (componentManagers_[family]) {
            // there is a ComponentManager for this type, do nothing.
            return;
        }

        // create a new ComponentManager for the type T
        componentManagers_[family] = std::make_unique<ComponentManager<T>>();
    }

    ///
    /// Get the ComponentManager for type T.
    /// \return Pointer to the ComponentManager
    ///
    template<typename T>
    ComponentManager<T>* ComponentFamilyManager::GetComponentMangerForFamily() const
    {
        std::uint32_t family = T::Family();
        if (!componentManagers_[family]) {
            // there is a ComponentManager for this type
            return nullptr;
        }

        // create a new ComponentManager for the type T
        return reinterpret_cast<ComponentManager<T>*>(componentManagers_[family].get());
    }

}
