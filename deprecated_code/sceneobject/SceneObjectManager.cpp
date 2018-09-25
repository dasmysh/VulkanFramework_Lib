/**
 * @file   SceneObjectManager.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Manager of all GameObject in the Engine.
 */

#include "SceneObjectManager.h"

#include <algorithm>

namespace vku {

    ///
    /// Creates a new GameObjectManager and creates a ComponentTypeManager for the
    /// Components of the GameObjects in this Manager.
    ///
    SceneObjectManager::SceneObjectManager() noexcept
    {
        // reserve some memory for SceneObject s and their component handles
        sceneObjects_.reserve(GAME_OBJECT_RESERVE_SIZE);
    }

    SceneObjectManager::SceneObjectManager(SceneObjectManager&& rhs) noexcept :
        sceneObjects_{ std::move(rhs.sceneObjects_) },
        freeSlots_{ std::move(rhs.freeSlots_) },
        componentFamilyManager_{ std::move(rhs.componentFamilyManager_) }
    {
    }

    SceneObjectManager& SceneObjectManager::operator=(SceneObjectManager&& rhs) noexcept
    {
        this->~SceneObjectManager();
        sceneObjects_ = std::move(rhs.sceneObjects_);
        freeSlots_ = std::move(rhs.freeSlots_);
        componentFamilyManager_ = std::move(rhs.componentFamilyManager_);
        return *this;
    }

    SceneObjectManager::~SceneObjectManager()
    {
        sceneObjects_.clear();
    }

    ///
    /// Creates a new GameObject with the given id.
    /// \return new GameObject
    ///
    SceneObject& SceneObjectManager::Create()
    {
        auto freeSlot = PopFreeSlot();

        if (freeSlot) {
            // make a new pair in the GameObject-List at the free slot
            // GameObject new_object(component_type_manager_.get(), id, free_slot);
            // game_objects_[free_slot.index] = std::move(new_object);
            sceneObjects_.emplace(sceneObjects_.begin() + freeSlot,
                &componentFamilyManager_, freeSlot);
            return sceneObjects_[freeSlot];
        }

        // allocate a new slot
        auto index = static_cast<std::uint32_t>(sceneObjects_.size());
        sceneObjects_.emplace_back(&componentFamilyManager_, index);
        return sceneObjects_[index];
    }

    void SceneObjectManager::Destroy(SceneObject& sceneObject)
    {
        auto sceneObjectHandle = sceneObject.GetHandle();
        if (sceneObjectHandle != std::numeric_limits<std::uint32_t>::max()) {
            sceneObjects_[sceneObjectHandle] = SceneObject();
            freeSlots_.push_back(sceneObjectHandle);
        }
    }

    ///
    /// Pops a free slot from the list of free slots.
    /// \return Handle to the free slot, if there is a free slot. Otherwise an
    /// invalid handle.
    ///
    std::uint32_t SceneObjectManager::PopFreeSlot() noexcept {
        if (HasFreeSlots()) {
            auto to_return = freeSlots_.back();
            freeSlots_.pop_back();
            return to_return;
        }
        return std::numeric_limits<std::uint32_t>::max();
    }
}
