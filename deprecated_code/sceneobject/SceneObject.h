/**
 * @file   SceneObject.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Generic representation of a SceneObject as described in 'Game Engine
 * Architecture - Second Edition, Jason Gregory, Page: 885'.
 */

#pragma once

#include "component/ComponentFamilyManager.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vku {

    /// Maximum numer of components per SceneObject
    constexpr std::uint32_t MAX_COMPONENTS = 32;

    ///
    /// Representation of an object in the game.
    /// This implementation tries to stay as close as possible to the implementation
    /// as shown in the book 'Game Engine Architecture - Second Edition, Jason
    /// Gregory, chapter 15.5.3.'
    /// FIXME: Currently this type of storing ComponentHandles in an array, where
    /// the position in the array is determined by the type of component, only
    /// supports having ONE component of each type of component.
    /// This also has the downside of having a fixed array of 32 handles (currently
    /// 32 bit) in each GameObject.
    ///
    class SceneObject
    {
    public:
        SceneObject() noexcept;
        SceneObject(ComponentFamilyManager*) noexcept;
        SceneObject(ComponentFamilyManager*, std::uint32_t) noexcept;

        SceneObject(const SceneObject&);
        SceneObject& operator=(const SceneObject&);
        SceneObject(SceneObject&&) noexcept;
        SceneObject& operator=(SceneObject&&) noexcept;
        ~SceneObject();

        std::uint32_t GetHandle() const noexcept { return sceneObjectHandle_; }
        void UpdateHandle(std::uint32_t handle) noexcept { sceneObjectHandle_ = handle; };

        template<typename T, typename... Args> void SetComponent(Args&&...);
        template<typename T> T* GetComponent() const;
        template<typename T> void RemoveComponent();
        template<typename T> bool HasComponent() const;

    private:
        /// Handle into the SceneObjectManager.
        std::uint32_t sceneObjectHandle_;
        /// Pointer to the ComponentManager
        /// note: we are not using a smart-pointer here, since this is just a
        /// reference and the GameObject does not own the ComponentManager
        ComponentFamilyManager* componentFamilyManager_;

        /// Handles to all components of this SceneObject
        std::array<std::uint32_t, MAX_COMPONENTS> componentHandles_;
    };

    ///
    /// Adds a Component to the GameObject. The Type of the Component to add is
    /// given by the first template-parameter. If the GameObject already has a
    /// component, update the component.
    /// \param Arguments to be forwarded to the constructor of the Component
    ///
    template<typename T, typename... Args>
    void SceneObject::SetComponent(Args&&... args) {

        std::uint32_t family = T::Family();

        if (HasComponent<T>()) {
            // handle to the component to be updated
            std::uint32_t componentHandle = componentHandles_[family];
            componentFamilyManager_->UpdateComponent<T>(
                sceneObjectHandle_, componentHandle, std::forward<Args>(args)...);
            return;
        }

        componentHandles_[family] = componentFamilyManager_->SetComponent<T>(
            sceneObjectHandle_, std::forward<Args>(args)...);
    }

    ///
    /// Gets the Component with the Type of the template-parameter from the
    /// GameObject.
    /// \return Component with the given type
    ///
    template<typename T> T* SceneObject::GetComponent() const {

        std::uint32_t family = T::Family();
        std::uint32_t componentHandle = componentHandles_[family];
        return componentFamilyManager_->GetComponent<T>(componentHandle);
    }

    ///
    /// Removes the Component with the Type of the template-parameter from the
    /// GameObject.
    ///
    template<typename T> void SceneObject::RemoveComponent() {

        std::uint32_t family = T::Family();
        std::uint32_t componentHandle = componentHandles_[family];
        componentHandles_[family] = std::numeric_limits<std::uint32_t>::max();
        componentFamilyManager_->RemoveComponent<T>(componentHandle);
    }

    ///
    /// Checks, if the GameObject has a Component of type T
    /// \return true, if the GameObject has this Component, otherwise false
    ///
    template<typename T> bool SceneObject::HasComponent() const {

        std::uint32_t family = T::Family();
        std::uint32_t componentHandle = componentHandles_[family];
        // All handles are initially invalid
        return componentHandle != std::numeric_limits<std::uint32_t>::max();
    }

}
