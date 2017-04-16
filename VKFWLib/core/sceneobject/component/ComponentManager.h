/**
 * @file   ComponentManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Manager for one type of components in the Engine.
 * This ComponentManager will be used for one type of Component like for
 * example AnimationComponent, CameraComponent, ...
 * This Managers actually is some kind of wrapper arround a std::vector which
 * can be accessed using a Handle into the vector. Free slots are stores into a
 * separate list, so free slots can be easily reused without walking the vector
 * itself.
 */

#pragma once

#include "Component.h"

#include <type_traits>
#include <vector>

namespace vku {

    /// Number of components to reserve in the component_data vector.
    constexpr std::uint32_t COMPONENT_RESERVE_SIZE = 64;

    /// Base class for a ComponentManager.
    class BaseComponentManager {
    public:
        virtual ~BaseComponentManager();
        bool HasFreeSlots() const noexcept { return freeSlots_.size() > 0; }
        virtual void RemoveComponent(std::uint32_t) = 0;
        virtual std::uint32_t CopyComponent(std::uint32_t) = 0;

    protected:
        std::uint32_t PopFreeSlot() noexcept;
        void PushFreeSlot(std::uint32_t componentHandle) { freeSlots_.push_back(componentHandle); }

    private:
        /// Contains handles to free slots.
        std::vector<std::uint32_t> freeSlots_;
    };

    template<typename T> class ComponentManager : public BaseComponentManager
    {
    public:
        ComponentManager() noexcept;

        T& operator[](const std::uint32_t) { return components_[handle]; }

        template<typename... Args> std::uint32_t SetComponent(std::uint32_t, Args&&...);
        template<typename... Args>
        void UpdateComponent(std::uint32_t, std::uint32_t, Args&&...);
        virtual void RemoveComponent(std::uint32_t) override;
        virtual std::uint32_t CopyComponent(std::uint32_t) override;

        const std::vector<T>& GetComponents() const noexcept { return components_; }

    private:
        /// Contains the actual components.
        std::vector<T> components_;
        /// Contains handles to free slots.
        std::vector<std::uint32_t> freeSlots_;
    };

    ///
    /// Constructor for a new ComponentManager.
    ///
    /// The constructor reserves some memory for COMPONENT_RESERVE_SIZE.
    ///
    template<typename T> ComponentManager<T>::ComponentManager() noexcept
    {
        static_assert(std::is_base_of<BaseComponent, T>::value,
            "T needs to be subclass of Component");
        components_.reserve(COMPONENT_RESERVE_SIZE);
    }

    ///
    /// Adds a new component to the ComponentManager and returns a handle to the new
    /// component.
    ///
    template<typename T>
    template<typename... Args>
    std::uint32_t ComponentManager<T>::SetComponent(std::uint32_t sceneObjectHandle,
        Args&&... args)
    {
        auto freeSlot = PopFreeSlot();

        if (freeSlot) {
            // The handle is valid
            // construct a new component at the location of the handle
            // TODO replace this one... this has two problems:
            // - this line is ugly
            // - you get weird compiler-errors, if you call SetComponent the wrong way
            //   (the component has matching constructor)
            new (&components_[freeSlot]) T(std::forward<Args>(args)...);
            components_[freeSlot].SetGameObjectHandle(sceneObjectHandle);
            return freeSlot;
        }

        // invalid handle
        // allocate a new slot
        auto index = static_cast<std::uint32_t>(components_.size());
        components_.emplace_back(std::forward<Args>(args)...);
        components_.back().SetGameObjectHandle(sceneObjectHandle);
        return index;
    }

    ///
    /// Updates the Component of a given GameObject at a given position with a new
    /// Component.
    ///
    /// \param Handle to the GameObject which the Component belongs to.
    /// \param Handle to the Component to replace.
    /// \param Arguments for the new Component
    ///
    template<typename T>
    template<typename... Args>
    void ComponentManager<T>::UpdateComponent(std::uint32_t sceneObjectHandle,
        std::uint32_t componentHandle, Args&&... args)
    {
        // TODO replace this one... see above
        new (&components_[componentHandle]) T(std::forward<Args>(args)...);
        // if we are call new, to create a new component, we also have to set the
        // game-object-handle again...
        // this is very ugly... TODO find a nicer solution to update a component,
        // without rely on having an additional constructor-parameter
        components_[componentHandle].SetGameObjectHandle(sceneObjectHandle);
    }

    ///
    /// Removes a Component from the Manager.
    ///
    /// \param Handle to the Component to remove
    ///
    template<typename T>
    void ComponentManager<T>::RemoveComponent(std::uint32_t componentHandle)
    {
        // call the components destructor to free slot.
        components_[componentHandle].~T();

        // add handle to free-list
        PushFreeSlot(componentHandle);
    }
    
    ///
    /// Copy the component behind the given handle and returns a 
    /// handle to the location of the new component.
    ///
    /// \param Location of the component to copy.
    /// \return Location to the location of the new component.
    ///
    template<typename T>
    std::uint32_t ComponentManager<T>::CopyComponent(std::uint32_t componentHandle)
    {
        auto freeSlot = PopFreeSlot();
        
        if (freeSlot) {
            components_[freeSlot] = components_[componentHandle];
            return freeSlot;
        }

        auto index = static_cast<std::uint32_t>(components_.size());
        components_.push_back(components_[componentHandle]);
        return index;
    }
}
