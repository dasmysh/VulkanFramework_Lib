/**
 * @file   SceneObject.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Generic representation of a GameObject as described in 'Game Engine
 * Architecture - Second Edition, Jason Gregory, Page: 885'.
 */

#include "SceneObject.h"

#include <algorithm>

namespace vku {

    ///
    /// Constructor for a GameObject.
    /// \param ComponentManager which holds the Components of the the GameObject
    /// \param Handle into the GameObjectManager
    ///
    SceneObject::SceneObject(ComponentFamilyManager* componentFamilyManager, std::uint32_t handle) noexcept :
        sceneObjectHandle_{ handle },
        componentFamilyManager_{ componentFamilyManager }
    {
        for (auto& componentHandle : componentHandles_) componentHandle = std::numeric_limits<std::uint32_t>::max();
    }

    ///
    /// Constructor for a invalid GameObject.
    ///
    SceneObject::SceneObject() noexcept : SceneObject{ nullptr, std::numeric_limits<std::uint32_t>::max() } {}

    ///
    /// Constructor for a GameObject.
    /// \param ComponentManager which holds the Components of the the GameObject
    ///
    SceneObject::SceneObject(ComponentFamilyManager* componentFamilyManager) noexcept :
        SceneObject{ componentFamilyManager, std::numeric_limits<std::uint32_t>::max() }
    {
    }

    ///
    /// Copy semantics for GameObjects
    ///
    SceneObject::SceneObject(const SceneObject& rhs) :
        sceneObjectHandle_{ rhs.sceneObjectHandle_ },
        componentFamilyManager_{ rhs.componentFamilyManager_ },
        componentHandles_{ rhs.componentHandles_ }
    {
        for (std::uint32_t family = 0; family < MAX_COMPONENTS; ++family) {
            if (componentHandles_[family]) {
                componentHandles_[family] = componentFamilyManager_->CopyComponent(family, componentHandles_[family]);
            }
        }
    }

    SceneObject& SceneObject::operator=(const SceneObject& rhs)
    {
        if (&rhs != this) {
            SceneObject tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    ///
    /// Move semantics for GameObjects
    ///
    SceneObject::SceneObject(SceneObject&& rhs) noexcept :
        sceneObjectHandle_{ rhs.sceneObjectHandle_ },
        componentFamilyManager_{ std::move(rhs.componentFamilyManager_) },
        componentHandles_{ std::move(rhs.componentHandles_) }
    {
    }

    SceneObject& SceneObject::operator=(SceneObject&& rhs) noexcept
    {
        this->~SceneObject();
        sceneObjectHandle_ = std::move(rhs.sceneObjectHandle_);
        componentFamilyManager_ = std::move(rhs.componentFamilyManager_);
        componentHandles_ = std::move(rhs.componentHandles_);
        return *this;
    }

    SceneObject::~SceneObject()
    {
        for (std::uint32_t family = 0; family < MAX_COMPONENTS; ++family) {
            auto componentHandle = componentHandles_[family];
            if (componentHandle != std::numeric_limits<std::uint32_t>::max()) componentFamilyManager_->RemoveComponent(family, componentHandle);
        }
    }
}
