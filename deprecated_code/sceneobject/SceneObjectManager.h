/**
 * @file   SceneObjectManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Manager of all SceneObject in the framework.
 */

#pragma once

#include "component/ComponentFamilyManager.h"
#include "SceneObject.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vku {

    /// Number of slots to reserver for GameObjects
    constexpr std::uint32_t GAME_OBJECT_RESERVE_SIZE = 1024;

    ///
    /// Manager of all the GameObjects in the Game.
    /// This class represents the HandleTable to the GameObjects as described in in
    /// the book 'Game Engine Architecture - Second Edition, Jason Gregory, chapter
    /// 15.5.3.'.
    ///
    class SceneObjectManager {

    public:
        SceneObjectManager() noexcept;
        SceneObjectManager(const SceneObjectManager&) = delete;
        SceneObjectManager& operator=(const SceneObjectManager&) = delete;
        SceneObjectManager(SceneObjectManager&&) noexcept;
        SceneObjectManager& operator=(SceneObjectManager&&) noexcept;
        ~SceneObjectManager();

        bool HasFreeSlots() const noexcept { return freeSlots_.size() > 0; }

        SceneObject& Create();
        void Destroy(SceneObject&);

        const SceneObject& FromHandle(std::uint32_t handle) const { return sceneObjects_[handle]; }

        // TODO we should return reference or pointer
        template<typename T>
        std::vector<std::uint32_t> FindSceneObjectsWithComponent() const;

    private:
        std::uint32_t PopFreeSlot() noexcept;

        /// List of GameObjects and Handles to Components of this GameObject
        /// TODO maybe use a hash-map or something like this, to ensure unique id of
        /// GameObjects.
        /// This would also improve search and would match chapter 15.5.4
        std::vector<SceneObject> sceneObjects_;
        /// Contains handles to free slots in game_objects_
        std::vector<std::uint32_t> freeSlots_;
        /// The ComponentManager
        ComponentFamilyManager componentFamilyManager_;
    };

    ///
    /// Find all GameObjects with a Component of type T.
    ///
    template<typename T>
    std::vector<std::uint32_t> SceneObjectManager::FindSceneObjectsWithComponent() const
    {
        const auto* componentManager =
            componentFamilyManager_.GetComponentMangerForFamily<T>();

        /// There is no ComponentManager for this type.
        if (!componentManager) { return std::vector<Handle>{}; }

        std::vector<std::uint32_t> sceneObjectHandles;

        // Iterate the components of the ComponentManager
        std::vector<T> components = componentManager->GetComponents();
        for (const auto& component : components) {
            // Get the handle of the GameObject in the Component and push the objects
            // into the result-vector
            sceneObjectHandles.push_back(component.GetGameObjectHandle());
        }

        return sceneObjectHandles;
    }

}
